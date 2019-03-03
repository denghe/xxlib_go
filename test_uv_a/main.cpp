#include "xx_uv.h"
namespace xx {
	// todo: GetIP func

	template<typename PeerType>
	struct UvUdpListener;

	struct UvUdpBasePeer {
		bool disposed = false;
		std::function<void()> OnDisconnect;
		sockaddr_in6 addr;						// set by Listener recv
		xx::Guid g;								// set by Listener recv
		uv_udp_t* uvUdp = nullptr;				// set by Listener recv
		char* recvBuf = nullptr;				// set by Listener recv
		int recvBufLen = 0;						// set by Listener recv
		ikcpcb* kcp = nullptr;					// need Init
		uint32_t nextUpdateTicks = 0;
		Buffer buf;

		inline void UpdateKcp(uint32_t const& current) noexcept {
			if (nextUpdateTicks > current) return;
			ikcp_update(kcp, current);
			nextUpdateTicks = ikcp_check(kcp, current);
			do {
				int recvLen = ikcp_recv(kcp, recvBuf, recvBufLen);
				if (recvLen <= 0) break;
				Unpack((uint8_t*)recvBuf, recvLen);
			} while (true);
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

		UvUdpBasePeer() = default;
		UvUdpBasePeer(UvUdpBasePeer const&) = delete;
		UvUdpBasePeer& operator=(UvUdpBasePeer const&) = delete;

		int Init();

		inline virtual void Dispose(bool callback = true) noexcept {
			if (disposed) return;
			disposed = true;
			if (callback && OnDisconnect) {
				OnDisconnect();
				OnDisconnect = nullptr;		// 如果 peer 被该 func 持有, 可能导致 peer 内存释放
			}
		}
	};

	template<typename PeerType>
	struct UvUdpListener : std::enable_shared_from_this<UvUdpListener<PeerType>> {
		uv_udp_t* uvUdp = nullptr;
		UvTimer_s updater;
		int64_t lastUpdateTime = std::chrono::steady_clock::now();
		std::function<std::shared_ptr<PeerType>()> OnCreatePeer;
		std::function<void(std::shared_ptr<PeerType>&& peer)> OnAccept;
		std::unordered_map<xx::Guid, std::weak_ptr<PeerType>> peers;
		std::vector<xx::Guid> dels;

		inline bool Disposed() noexcept {
			return uvUdp == nullptr;
		}
		inline int Init(UvLoop& loop) noexcept {
			if (uvUdp) return 0;
			updater = loop.CreateTimer(10, 10, [this] {
				auto nowTime = std::chrono::steady_clock::now();
				auto elapsedMS = std::chrono::milliseconds(nowTime - lastUpdateTime);
				lastUpdateTime = nowTime;
				for (decltype(auto) kv : peers) {
					if (auto peer = kv.value.lock()) {
						peer->UpdateKcp(elapsedMS);
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
			uvUdp = UvAlloc<uv_udp_t>(this);
			if (!uvUdp) return -1;
			if (int r = uv_udp_init(&loop.uvLoop, uvUdp)) {
				uvUdp = nullptr;
				return r;
			}
			return 0;
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

		virtual void HandleRecv(uint8_t* const& recvBuf, uint32_t const& recvLen, sockaddr* const& addr) noexcept {
			if (len < 36) return;			// header 至少有 36 字节长( Guid conv 的 kcp 头 )
			Guid g(false);
			g.Fill(bufPtr);					// 前 16 字节转为 Guid
			auto iter = peers.find(g);		// 去字典中找. 没有就新建.
			std::shared_ptr<PeerType> p;
			if (iter == peers.end()) {
				p = OnCreatePeer ? OnCreatePeer() : std::make_shared<PeerType>(*this, g);
				if (!p) return -1;
				peers[g] = p;
			}
			else {
				p = iter->second.lock();
				if (!p) {
					peers.erase(iter);
					return -1;
				}
			}
			memcpy(&p->addr, addr, sizeof(sockaddr_in6));	// 更新 peer 的目标 ip 地址		// todo: ip changed 事件?? 严格模式? ip 变化就清除 peer
			if (iter == peers.end() && OnAccept) {
				OnAccept(p);
			}
			if (p->disposed) return;
			if (ikcp_input(p->kcp, recvBuf, recvLen)) {
				p->Dispose();
			}
		}
	};

	inline int UvUdpBasePeer::Init() {
		kcp = ikcp_create(g, this);
		if (!kcp) return -1;
		xx::ScopeGuard sg_ptr([&]() noexcept { ikcp_release(kcp); kcp = nullptr; });

		int r = 0;
		if ((r = ikcp_wndsize(kcp, 128, 128))
			|| (r = ikcp_nodelay(kcp, 1, 10, 2, 1))) throw r;

		(kcp)->rx_minrto = 100;
		ikcp_setoutput(kcp, [](const char *inBuf, int len, ikcpcb *kcp, void *user)->int {
			auto peer = (UvUdpBasePeer*)user;
			struct uv_udp_send_t_ex : uv_udp_send_t {
				uv_buf_t buf;
			};
			auto req = (uv_udp_send_t_ex*)::malloc(sizeof(uv_udp_send_t_ex) + len);
			auto buf = (char*)(req + 1);
			memcpy(buf, inBuf, len);
			req->buf = uv_buf_init(buf, (uint32_t)len);
			return uv_udp_send(req, peer->uvUdp, &req->buf, 1, (sockaddr*)&peer->addr, [](uv_udp_send_t* req, int status) noexcept {
				::free(req);
			});
		});
	}

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
				UvGetSelf<UvUdpListener<PeerType>>(handle)->HandleRecv((uint8_t*)buf->base, (uint32_t)nread, addr);
			}
			if (buf) ::free(buf->base);
		})) return nullptr;

		return listener;
	}
}

int main(int argc, char* argv[]) {
	return 0;
}
