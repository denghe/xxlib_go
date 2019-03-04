#include "xx_uv.h"
namespace xx {
	// todo: GetIP func

	struct UvUdp : std::enable_shared_from_this<UvUdp> {
		uv_udp_t* uvUdp = nullptr;
		std::array<char, 65536> recvBuf;

		inline bool Disposed() noexcept {
			return uvUdp == nullptr;
		}
		inline virtual int Init(UvLoop& loop) noexcept {
			if (uvUdp) return 0;
			uvUdp = UvAlloc<uv_udp_t>(this);
			if (!uvUdp) return -1;
			if (int r = uv_udp_init(&loop.uvLoop, uvUdp)) {
				uvUdp = nullptr;
				return r;
			}
			return 0;
		}
		virtual ~UvUdp() {
			UvHandleCloseAndFree(uvUdp);
		}
	};

	struct UvUdpPeer : UvUdp {
		UvLoop& loop;
		std::function<void()> OnConnect;

		inline virtual int Init(UvLoop& loop) noexcept override {
			return this->UvUdp::Init(loop);
		}

		UvUdpPeer(UvLoop& loop) noexcept : loop(loop) {}
		UvUdpPeer(UvUdpPeer const&) = delete;
		UvUdpPeer& operator=(UvUdpPeer const&) = delete;

		void Dial(std::string const& ip, int const& port) {
			sockaddr_in6 addr;
			if (ip.find(':') == std::string::npos) {								// ipv4
				if (uv_ip4_addr(ip.c_str(), port, (sockaddr_in*)&addr)) return;
			}
			else {																	// ipv6
				if (uv_ip6_addr(ip.c_str(), port, &addr)) return;
			}
			if (uv_udp_bind(uvUdp, (sockaddr*)&addr, UV_UDP_REUSEADDR)) return;

			if (uv_udp_recv_start(uvUdp, UvAllocCB, [](uv_udp_t* handle, ssize_t nread, const uv_buf_t* buf, const struct sockaddr* addr, unsigned flags) {
				if (nread > 0) {
					UvGetSelf<UvUdpPeer>(handle)->Unpack((uint8_t*)buf->base, (uint32_t)nread, addr);
				}
				if (buf) ::free(buf->base);
			})) return;
		}

		virtual void Unpack(uint8_t* const& recvBuf, uint32_t const& recvLen, sockaddr const* const& addr) noexcept = 0;
	};



	struct UvUdpKcpPeer {
		ikcpcb* kcp = nullptr;					// need Init
		uint32_t nextUpdateTicks = 0;
		sockaddr_in6 addr;						// set by Listener recv every time
		Buffer buf;
		UvUdp* owner;			// set by Listener
		std::function<void()> OnDisconnect;

		inline int UpdateKcp(uint32_t const& current) noexcept {
			if (nextUpdateTicks > current) return 0;
			ikcp_update(kcp, current);
			nextUpdateTicks = ikcp_check(kcp, current);
			do {
				int recvLen = ikcp_recv(kcp, owner->recvBuf.data(), (int)owner->recvBuf.size());
				if (recvLen <= 0) break;
				if (int r = Unpack((uint8_t*)owner->recvBuf.data(), recvLen)) return r;
			} while (true);
			return 0;
		}

		virtual int Unpack(uint8_t* const& recvBuf, uint32_t const& recvLen) noexcept {
			buf.AddRange(recvBuf, recvLen);
			uint32_t offset = 0;
			while (offset + 4 <= buf.len) {							// ensure header len( 4 bytes )
				auto len = buf[offset + 0] + (buf[offset + 1] << 8) + (buf[offset + 2] << 16) + (buf[offset + 3] << 24);
				if (len <= 0 /* || len > maxLimit */) return -1;	// invalid length
				if (offset + 4 + len > buf.len) break;				// not enough data

				offset += 4;
				if (int r = HandlePack(buf.buf + offset, len)) return r;
				offset += len;
			}
			buf.RemoveFront(offset);
			return 0;
		}

		inline virtual int HandlePack(uint8_t* const& recvBuf, uint32_t const& recvLen) noexcept { return 0; };

		UvUdpKcpPeer() = default;
		UvUdpKcpPeer(UvUdpKcpPeer const&) = delete;
		UvUdpKcpPeer& operator=(UvUdpKcpPeer const&) = delete;

		inline int Init(UvUdp* owner, xx::Guid const& g) {
			this->owner = owner;
			kcp = ikcp_create(g, this);
			if (!kcp) return -1;
			xx::ScopeGuard sg_kcp([&] { ikcp_release(kcp); kcp = nullptr; });
			if (int r = ikcp_wndsize(kcp, 128, 128)) return r;
			if (int r = ikcp_nodelay(kcp, 1, 10, 2, 1)) return r;

			(kcp)->rx_minrto = 100;
			ikcp_setoutput(kcp, [](const char *inBuf, int len, ikcpcb *kcp, void *user)->int {
				auto peer = (UvUdpKcpPeer*)user;
				struct uv_udp_send_t_ex : uv_udp_send_t {
					uv_buf_t buf;
				};
				auto req = (uv_udp_send_t_ex*)::malloc(sizeof(uv_udp_send_t_ex) + len);
				auto buf = (char*)(req + 1);
				memcpy(buf, inBuf, len);
				req->buf = uv_buf_init(buf, (uint32_t)len);
				return uv_udp_send(req, peer->owner->uvUdp, &req->buf, 1, (sockaddr*)&peer->addr, [](uv_udp_send_t* req, int status) noexcept {
					::free(req);
				});
			});
			sg_kcp.Cancel();
		}

		inline int Input(char const* const& buf, int const& len) {
			if (!kcp) return -1;
			return ikcp_input(kcp, buf, len);
		}

		inline virtual void Dispose(bool callback = true) noexcept {
			if (!kcp) return;
			ikcp_release(kcp);
			kcp = nullptr;
			if (callback && OnDisconnect) {
				OnDisconnect();
				OnDisconnect = nullptr;		// 如果 peer 被该 func 持有, 可能导致 peer 内存释放
			}
		}
	};

	template<typename PeerType>
	struct UvUdpListener : UvUdp {
		UvTimer_s updater;
		int64_t lastUpdateTime = std::chrono::steady_clock::now();
		std::function<std::shared_ptr<PeerType>()> OnCreatePeer;
		std::function<void(std::shared_ptr<PeerType>&& peer)> OnAccept;
		std::unordered_map<xx::Guid, std::weak_ptr<PeerType>> peers;
		std::vector<xx::Guid> dels;

		inline virtual int Init(UvLoop& loop) noexcept override {
			if (uvUdp) return 0;
			updater = loop.CreateTimer(10, 10, [this] {
				auto nowTime = std::chrono::steady_clock::now();
				auto elapsedMS = std::chrono::milliseconds(nowTime - lastUpdateTime);
				lastUpdateTime = nowTime;
				for (decltype(auto) kv : peers) {
					if (auto peer = kv.value.lock()) {
						if (peer->UpdateKcp(elapsedMS)) {
							peer->Dispose();
							dels.push_back(kv.first);
						}
					}
					else {
						dels.push_back(kv.first);
					}
				}
				for (decltype(auto) g : dels) {
					peers.erase(g);
				}
				dels.clear();
			});
			if (!updater) return -1;
			return this->UvUdp::Init(loop);
		}
		
		UvUdpListener() = default;
		UvUdpListener(UvUdpListener const&) = delete;
		UvUdpListener& operator=(UvUdpListener const&) = delete;
		virtual ~UvUdpListener() {
			for (decltype(auto) kv : peers) {
				if (auto peer = kv.second.lock()) {
					peer->Dispose();
				}
			}
			UvHandleCloseAndFree(uvUdp);
		}

		inline virtual std::shared_ptr<PeerType> CreatePeer() noexcept {
			return OnCreatePeer ? OnCreatePeer() : std::make_shared<PeerType>();
		}
		inline virtual void Accept(std::shared_ptr<PeerType>&& peer) noexcept {
			if (OnAccept) {
				OnAccept(std::move(peer));
			}
		};

		virtual void Unpack(uint8_t* const& recvBuf, uint32_t const& recvLen, sockaddr* const& addr) noexcept {
			if (len < 36) return;			// header 至少有 36 字节长( Guid conv 的 kcp 头 )
			Guid g(false);
			g.Fill(bufPtr);					// 前 16 字节转为 Guid
			auto iter = peers.find(g);		// 去字典中找. 没有就新建.
			std::shared_ptr<PeerType> p = iter == peers.end() ? nullptr : iter->second.lock();
			if (!p) {
				p = OnCreatePeer ? OnCreatePeer() : std::make_shared<PeerType>();
				if (!p || p->Init(this, g)) return;
				peers[g] = p;
			}
			memcpy(&p->addr, addr, sizeof(sockaddr_in6));	// 更新 peer 的目标 ip 地址		// todo: ip changed 事件?? 严格模式? ip 变化就清除 peer
			if (iter == peers.end() && OnAccept) {
				OnAccept(p);
			}
			if (p->Disposed()) return;
			if (p->Input(recvBuf, recvLen)) {
				p->Dispose();
			}
		}
	};

	template<typename PeerType>
	std::shared_ptr<UvUdpListener<PeerType>> CreateUdpListener(UvLoop& loop, std::string const& ip, int const& port) {
		auto listener = std::make_shared<ListenerType>();
		if (listener->Init(&loop.uvLoop)) return nullptr;

		sockaddr_in6 addr;
		if (ip.find(':') == std::string::npos) {								// ipv4
			if (uv_ip4_addr(ip.c_str(), port, (sockaddr_in*)&addr)) return nullptr;
		}
		else {																	// ipv6
			if (uv_ip6_addr(ip.c_str(), port, &addr)) return nullptr;
		}
		if (uv_udp_bind(listener->uvUdp, (sockaddr*)&addr, UV_UDP_REUSEADDR)) return nullptr;

		if(uv_udp_recv_start(listener->uvUdp, UvAllocCB, [](uv_udp_t* handle, ssize_t nread, const uv_buf_t* buf, const struct sockaddr* addr, unsigned flags) {
			if (nread > 0) {
				UvGetSelf<UvUdpListener<PeerType>>(handle)->Unpack((uint8_t*)buf->base, (uint32_t)nread, addr);
			}
			if (buf) ::free(buf->base);
		})) return nullptr;

		return listener;
	}

	template<typename PeerType>
	struct UvUdpDialer : UvUdp {
		using PeerType_s = std::shared_ptr<PeerType>;
		UvLoop& loop;
		UvTimer_s updater;
		int64_t lastUpdateTime = std::chrono::steady_clock::now();
		std::function<PeerType_s()> OnCreatePeer;
		std::function<void()> OnConnect;
		xx::Guid g;
		PeerType_s peer;

		inline virtual int Init(UvLoop& loop) noexcept override {
			if (uvUdp) return 0;
			updater = loop.CreateTimer(10, 10, [this] {
				auto nowTime = std::chrono::steady_clock::now();
				auto elapsedMS = std::chrono::milliseconds(nowTime - lastUpdateTime);
				lastUpdateTime = nowTime;
				if (peer) {
					if (peer->UpdateKcp(elapsedMS)) {
						peer->Dispose();
						peer.reset();
					}
				}
			});
			if (!updater) return -1;
			return this->UvUdp::Init(loop);
		}

		UvUdpDialer(UvLoop& loop) noexcept : loop(loop) {}
		UvUdpDialer(UvUdpDialer const&) = delete;
		UvUdpDialer& operator=(UvUdpDialer const&) = delete;

		virtual ~UvUdpDialer() {
			if (peer) {
				peer->Dispose();
				peer.reset();
			}
		}

		inline virtual PeerType_s CreatePeer() noexcept {
			return OnCreatePeer ? OnCreatePeer() : std::make_shared<PeerType>();
		}
		inline virtual void Connect() noexcept {
			if (OnConnect) {
				OnConnect();
			}
		}

		// todo: 模拟握手？ peer 创建之后发送握手包并等待返回？
		// todo: 批量拨号？
		// todo: 实现一个与 kcp 无关的握手流程？只发送 guid? 直接返回 guid? 使用一个 timer 来管理? 不停的发，直到收到？

		void Dial(std::string const& ip, int const& port) {
			g.Fill();
			peer = CreatePeer();
			xx::ScopeGuard sgPeer([&] { peer.reset(); });
			if (peer->Init(this, g)) return;

			sockaddr_in6 addr;
			if (ip.find(':') == std::string::npos) {								// ipv4
				if (uv_ip4_addr(ip.c_str(), port, (sockaddr_in*)&addr)) return;
			}
			else {																	// ipv6
				if (uv_ip6_addr(ip.c_str(), port, &addr)) return;
			}
			if (uv_udp_bind(peer->uvUdp, (sockaddr*)&addr, UV_UDP_REUSEADDR)) return;

			if (uv_udp_recv_start(peer->uvUdp, UvAllocCB, [](uv_udp_t* handle, ssize_t nread, const uv_buf_t* buf, const struct sockaddr* addr, unsigned flags) {
				if (nread > 0) {
					UvGetSelf<UvUdpDialer<PeerType>>(handle)->Unpack((uint8_t*)buf->base, (uint32_t)nread, addr);
				}
				if (buf) ::free(buf->base);
			})) return;

			sgPeer.Cancel();
			Connect();
		}

		virtual void Unpack(uint8_t* const& recvBuf, uint32_t const& recvLen, sockaddr* const& addr) noexcept {
			if (!peer || peer->Disposed()) return;		// 没建立对端
			// todo: 校验 addr 是否与 Dial 时传递的一致
			if (len < 36) return;						// header 至少有 36 字节长( Guid conv 的 kcp 头 )
			Guid g(false);
			g.Fill(bufPtr);								// 前 16 字节转为 Guid
			if (this->g != g) return;					// 有可能是延迟收到上个连接的残包
			if (peer->Input(recvBuf, recvLen)) {
				peer->Dispose();
			}
		}
	};
}

int main(int argc, char* argv[]) {
	return 0;
}
