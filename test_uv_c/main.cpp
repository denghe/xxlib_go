#include "xx_uv.h"
namespace xx {
	struct UvUdpKcpPeer : UvItem {
		std::function<void()> OnDisconnect;
		std::function<int(Object_s&& msg)> OnReceivePush;
		std::function<int(int const& serial, Object_s&& msg)> OnReceiveRequest;
		std::unordered_map<int, std::pair<std::function<int(Object_s&& msg)>, UvTimer_s>> callbacks;
		int serial = 0;
		ikcpcb* kcp = nullptr;
		uint32_t nextUpdateTicks = 0;
		UvUdpBasePeer* owner;	// for Send
		Buffer buf;
		sockaddr_in6 addr;		// for Send. fill by owner Unpack

		UvUdpKcpPeer(UvUdpBasePeer* owner, Guid const& g)
			: UvItem(owner->loop)
			, owner(owner) {
			kcp = ikcp_create(g, this);
			if (!kcp) throw - 1;
			xx::ScopeGuard sgKcp([&] { ikcp_release(kcp); kcp = nullptr; });
			if (int r = ikcp_wndsize(kcp, 128, 128)) throw r;
			if (int r = ikcp_nodelay(kcp, 1, 10, 2, 1)) throw r;
			(kcp)->rx_minrto = 100;
			ikcp_setoutput(kcp, [](const char *inBuf, int len, ikcpcb *kcp, void *user)->int {
				auto self = ((UvUdpKcpPeer*)user);
				return self->owner->Send(inBuf, len, (sockaddr*)&self->addr);
			});
			sgKcp.Cancel();
		}
		UvUdpKcpPeer(UvUdpKcpPeer const&) = delete;
		UvUdpKcpPeer& operator=(UvUdpKcpPeer const&) = delete;

		~UvUdpKcpPeer() {
			if (kcp) {
				ikcp_release(kcp);
				kcp = nullptr;
			}
		}

		inline bool Disposed() const noexcept {
			return !kcp;
		}

		inline virtual void Dispose(bool callback = true) noexcept {
			if (!kcp) return;
			ikcp_release(kcp);
			kcp = nullptr;
			for (decltype(auto) kv : callbacks) {
				kv.second.first(nullptr);
			}
			callbacks.clear();
			auto self = shared_from_this();		// hold memory
			OnReceivePush = nullptr;
			OnReceiveRequest = nullptr;
			if (callback && OnDisconnect) {
				OnDisconnect();
				OnDisconnect = nullptr;		// 如果 peer 被该 func 持有, 可能导致 peer 内存释放
			}
		}

		inline int Input(uint8_t* const& recvBuf, uint32_t const& recvLen) noexcept {
			if (!kcp) return -1;
			return ikcp_input(kcp, (char*)recvBuf, recvLen);
		}

		inline int Update(uint32_t const& current) noexcept {
			if (nextUpdateTicks > current) return 0;
			ikcp_update(kcp, current);
			nextUpdateTicks = ikcp_check(kcp, current);
			do {
				int recvLen = ikcp_recv(kcp, loop.recvBuf.data(), (int)loop.recvBuf.size());
				if (recvLen <= 0) break;
				if (int r = Unpack((uint8_t*)loop.recvBuf.data(), recvLen)) return r;
			} while (true);
			return 0;
		}

		inline virtual int Unpack(uint8_t* const& recvBuf, uint32_t const& recvLen) noexcept {
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

		inline virtual int HandlePack(uint8_t* const& recvBuf, uint32_t const& recvLen) noexcept {
			auto& recvBB = loop.recvBB;
			recvBB.Reset((uint8_t*)recvBuf, recvLen);

			int serial = 0;
			if (int r = recvBB.Read(serial)) return r;
			Object_s msg;
			if (int r = recvBB.ReadRoot(msg)) return r;

			if (serial == 0) {
				return OnReceivePush ? OnReceivePush(std::move(msg)) : 0;
			}
			else if (serial < 0) {
				return OnReceiveRequest ? OnReceiveRequest(-serial, std::move(msg)) : 0;
			}
			else {
				auto iter = callbacks.find(serial);
				if (iter == callbacks.end()) return 0;
				int r = iter->second.first(std::move(msg));
				callbacks.erase(iter);
				return r;
			}
		}

		inline int Send(uint8_t const* const& buf, ssize_t const& dataLen) noexcept {
			if (!kcp) return -1;
			return ikcp_send(kcp, (char*)buf, (int)dataLen);
		}

		// serial == 0: push    > 0: response    < 0: request
		inline int SendPackage(Object_s const& data, int32_t const& serial = 0, int const& tar = 0) {
			if (!kcp) return -1;
			auto& sendBB = loop.sendBB;
			sendBB.Resize(4);						// header len( 4 bytes )
			if (tar) sendBB.WriteFixed(tar);
			sendBB.Write(serial);
			sendBB.WriteRoot(data);
			auto buf = sendBB.buf;
			auto len = sendBB.len - 4;
			buf[0] = uint8_t(len);					// fill package len
			buf[1] = uint8_t(len >> 8);
			buf[2] = uint8_t(len >> 16);
			buf[3] = uint8_t(len >> 24);
			return Send(buf, sendBB.len);
		}

		inline int SendPush(Object_s const& data, int const& tar = 0) {
			return SendPackage(data, 0, tar);
		}

		inline int SendResponse(int32_t const& serial, Object_s const& data, int const& tar = 0) {
			return SendPackage(data, serial, tar);
		}

		inline int SendRequest(Object_s const& data, int const& tar, std::function<int(Object_s&& msg)>&& cb, uint64_t const& timeoutMS = 0) {
			if (!kcp) return -1;
			std::pair<std::function<int(Object_s&& msg)>, UvTimer_s> v;
			++serial;
			if (timeoutMS) {
				v.second = xx::TryMake<UvTimer>(loop, timeoutMS, 0, [this, serial = this->serial]() {
					TimeoutCallback(serial);
				});
				if (!v.second) return -1;
			}
			if (int r = SendPackage(data, -serial, tar)) return r;
			v.first = std::move(cb);
			callbacks[serial] = std::move(v);
			return 0;
		}
		inline int SendRequest(Object_s const& msg, std::function<int(Object_s&& msg)>&& cb, uint64_t const& timeoutMS = 0) {
			return SendRequest(msg, 0, std::move(cb), timeoutMS);
		}

		inline void TimeoutCallback(int const& serial) {
			auto iter = callbacks.find(serial);
			if (iter == callbacks.end()) return;
			iter->second.first(nullptr);
			callbacks.erase(iter);
		}
	};
	using UvUdpKcpPeer_s = std::shared_ptr<UvUdpKcpPeer>;
	using UvUdpKcpPeer_w = std::weak_ptr<UvUdpKcpPeer>;

	template<typename PeerType>
	struct UvUdpBasePeerKcpEx : UvUdpBasePeer {
		using PeerType_s = std::shared_ptr<PeerType>;
		UvTimer_s updater;
		std::chrono::steady_clock::time_point lastUpdateTime = std::chrono::steady_clock::now();
		std::function<PeerType_s(UvUdpBasePeer* owner, Guid const& g)> OnCreatePeer;

		using UvUdpBasePeer::UvUdpBasePeer;
		inline virtual PeerType_s CreatePeer(Guid const& g) noexcept {
			return OnCreatePeer ? OnCreatePeer(this, g) : xx::TryMake<PeerType>(this, g);
		}
	};

	template<typename PeerType = UvUdpKcpPeer>
	struct UvUdpKcpListener : UvUdpBasePeerKcpEx<PeerType> {
		std::function<void(PeerType_s& peer)> OnAccept;
		std::unordered_map<xx::Guid, std::weak_ptr<PeerType>> peers;
		std::vector<xx::Guid> dels;

		UvUdpKcpListener(UvLoop& loop, std::string const& ip, int const& port)
			: UvUdpBasePeerKcpEx<PeerType>(loop, ip, port, true) {
			updater = xx::Make<UvTimer>(loop, 10, 10, [this] {
				auto nowTime = std::chrono::steady_clock::now();
				auto elapsedMS = std::chrono::duration_cast<std::chrono::milliseconds>(nowTime - lastUpdateTime).count();
				lastUpdateTime = nowTime;
				for (decltype(auto) kv : peers) {		// todo: timeout check?
					if (auto peer = kv.second.lock()) {
						if (peer->Update((int)elapsedMS)) {
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
		}
		UvUdpKcpListener(UvUdpKcpListener const&) = delete;
		UvUdpKcpListener& operator=(UvUdpKcpListener const&) = delete;

		virtual ~UvUdpKcpListener() {
			for (decltype(auto) kv : peers) {
				if (auto peer = kv.second.lock()) {
					peer->Dispose();
				}
			}
		}

		virtual int Unpack(uint8_t* const& recvBuf, uint32_t const& recvLen, sockaddr const* const& addr) noexcept override {
			if (recvLen < 36) return 0;		// header 至少有 36 字节长( Guid conv 的 kcp 头 )
			Guid g(false);
			g.Fill(recvBuf);				// 前 16 字节转为 Guid
			auto iter = peers.find(g);		// 去字典中找. 没有就新建.
			PeerType_s p = iter == peers.end() ? nullptr : iter->second.lock();
			if (!p) {
				p = CreatePeer(g);
				if (!p) return 0;
				peers[g] = p;
			}
			memcpy(&p->addr, addr, sizeof(sockaddr_in6));	// 更新 peer 的目标 ip 地址		// todo: ip changed 事件?? 严格模式? ip 变化就清除 peer
			if (iter == peers.end() && OnAccept) {
				OnAccept(p);
			}
			if (p->Disposed()) return 0;
			if (p->Input(recvBuf, recvLen)) {
				p->Dispose();
			}
			return 0;
		}
	};

	template<typename PeerType = UvUdpKcpPeer>
	struct UvUdpKcpDialer : UvUdpBasePeerKcpEx<PeerType> {
		std::function<void()> OnConnect;
		xx::Guid g;
		PeerType_s peer;

		UvUdpKcpDialer(UvLoop& loop)
			: UvUdpBasePeerKcpEx<PeerType>(loop, "", 0, false) {
			updater = xx::Make<UvTimer>(loop, 10, 10, [this] {
				auto nowTime = std::chrono::steady_clock::now();
				auto elapsed = nowTime - lastUpdateTime;
				auto elapsedMS = std::chrono::duration_cast<std::chrono::milliseconds>(nowTime - lastUpdateTime).count();
				lastUpdateTime = nowTime;
				if (peer) {
					if (peer->Update((int)elapsedMS)) {
						peer->Dispose();
						peer.reset();
					}
				}
			});
		}
		UvUdpKcpDialer(UvUdpKcpDialer const&) = delete;
		UvUdpKcpDialer& operator=(UvUdpKcpDialer const&) = delete;

		virtual ~UvUdpKcpDialer() {
			if (peer) {
				peer->Dispose();
				peer.reset();
			}
		}

		inline virtual void Connect() noexcept {
			if (OnConnect) {
				OnConnect();
			}
		};


		// todo: 模拟握手？ peer 创建之后发送握手包并等待返回？
		// todo: 批量拨号？
		// todo: 实现一个与 kcp 无关的握手流程？只发送 guid? 直接返回 guid? 使用一个 timer 来管理? 不停的发，直到收到？

		int Dial(std::string const& ip, int const& port) noexcept {
			g.Gen();
			peer = CreatePeer(g);
			if (!peer) return -1;
			xx::ScopeGuard sgPeer([&] { peer.reset(); });

			if (ip.find(':') == std::string::npos) {								// ipv4
				if (int r = uv_ip4_addr(ip.c_str(), port, (sockaddr_in*)&peer->addr)) return r;
			}
			else {																	// ipv6
				if (int r = uv_ip6_addr(ip.c_str(), port, &peer->addr)) return r;
			}

			sgPeer.Cancel();
			Connect();
		}

		virtual int Unpack(uint8_t* const& recvBuf, uint32_t const& recvLen, sockaddr const* const& addr) noexcept override {
			if (!peer || peer->Disposed()) return 0;	// 没建立对端
			// todo: 校验 addr 是否与 Dial 时传递的一致
			if (recvLen < 36) return 0;					// header 至少有 36 字节长( Guid conv 的 kcp 头 )
			Guid g(false);
			g.Fill(recvBuf);							// 前 16 字节转为 Guid
			if (this->g != g) return 0;					// 有可能是延迟收到上个连接的残包
			if (peer->Input(recvBuf, recvLen)) {
				peer->Dispose();
			}
			return 0;
		}
	};
}

int main(int argc, char* argv[]) {
	std::cout << "begin\n";
	for (size_t i = 0; i < 30; i++)	{
		xx::UvLoop loop;
		auto listener = xx::TryMake<xx::UvUdpKcpListener<>>(loop, "0.0.0.0", 12345);
		assert(listener);
		listener->OnAccept = [](xx::UvUdpKcpPeer_s& peer) {
			peer->OnReceivePush = [peer](xx::Object_s&& msg)->int {
				std::cout << "recv " << msg << std::endl;
				return 0;
			};
		};

		auto timer = xx::TryMake<xx::UvTimer>(loop, 50, 0);
		assert(timer);

		auto dialer = xx::TryMake<xx::UvUdpKcpDialer<>>(loop);
		assert(dialer);

		timer->OnFire = [&] {
			dialer->OnConnect = [&] {
				auto msg = xx::TryMake<xx::BBuffer>();
				msg->Write(1, 2, 3, 4, 5);
				dialer->peer->SendPush(msg);
				std::cout << "send " << msg << std::endl;
				timer->OnFire = [&] {
					loop.Stop();
				};
				timer->Restart();
			};
			dialer->Dial("127.0.0.1", 12345);
		};
		loop.Run();
	}
	return 0;
}
