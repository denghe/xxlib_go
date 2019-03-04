#pragma once
#include "uv.h"
#include "xx_bbuffer.h"
#include "ikcp.h"

// todo: CreateXxxxx 放到外面来. 方便扩展

// 重要：
// std::function 的捕获列表通常不可以随意增加引用以导致无法析构, 尽量使用 weak_ptr. 
// 当前唯有 UvTcpPeer OnDisconnect, OnReceivePush, OnReceiveRequest 可以使用 shared_ptr 捕获, 会在 Dispose 时自动清除

namespace xx {
	struct UvTimer;
	using UvTimer_s = std::shared_ptr<UvTimer>;
	using UvTimer_w = std::weak_ptr<UvTimer>;
	struct UvResolver;
	using UvResolver_s = std::shared_ptr<UvResolver>;
	using UvResolver_w = std::weak_ptr<UvResolver>;
	struct UvAsync;
	using UvAsync_s = std::shared_ptr<UvAsync>;
	using UvAsync_w = std::weak_ptr<UvAsync>;
	struct UvTcpPeer;
	using UvTcpPeer_s = std::shared_ptr<UvTcpPeer>;
	using UvTcpPeer_w = std::weak_ptr<UvTcpPeer>;
	template<typename PeerType = UvTcpPeer>
	struct UvTcpListener;

	template<typename T>
	T* UvAlloc(void* const& ud) noexcept {
		auto p = (void**)::malloc(sizeof(void*) + sizeof(T));
		if (!p) return nullptr;
		p[0] = ud;
		return (T*)&p[1];
	}
	inline void UvFree(void* const& p) noexcept {
		::free((void**)p - 1);
	}
	template<typename T>
	T* UvGetSelf(void* const& p) noexcept {
		return (T*)*((void**)p - 1);
	}

	template<typename T>
	void UvHandleCloseAndFree(T*& tar) noexcept {
		if (!tar) return;
		auto h = (uv_handle_t*)tar;
		tar = nullptr;
		assert(!uv_is_closing(h));
		uv_close(h, [](uv_handle_t* handle) {
			UvFree(handle);
		});
	}
	static void UvAllocCB(uv_handle_t* h, size_t suggested_size, uv_buf_t* buf) noexcept {
		buf->base = (char*)::malloc(suggested_size);
		buf->len = decltype(uv_buf_t::len)(suggested_size);
	}

	struct UvLoop {
		uv_loop_t uvLoop;
		UvLoop() {
			if (int r = uv_loop_init(&uvLoop)) throw r;
		}
		UvLoop(UvLoop const&) = delete;
		UvLoop& operator=(UvLoop const&) = delete;

		inline int Run(uv_run_mode const& mode = UV_RUN_DEFAULT) noexcept {
			return uv_run(&uvLoop, mode);
		}
	};

	// make + init + start
	template<typename TimerType = UvTimer, typename ENABLED = std::enable_if_t<std::is_base_of_v<UvTimer, TimerType>>>
	std::shared_ptr<TimerType> CreateUvTimer(UvLoop& loop, uint64_t const& timeoutMS, uint64_t const& repeatIntervalMS, std::function<void()>&& onFire = nullptr) noexcept;

	struct UvAsync {
		std::mutex mtx;
		std::deque<std::function<void()>> actions;
		std::function<void()> action;
		uv_async_t* uvAsync = nullptr;

		UvAsync() = default;
		UvAsync(UvAsync const&) = delete;
		UvAsync& operator=(UvAsync const&) = delete;

		virtual ~UvAsync() {
			UvHandleCloseAndFree(uvAsync);
		}
		inline int Init(uv_loop_t* const& loop) noexcept {
			if (uvAsync) return 0;
			uvAsync = UvAlloc<uv_async_t>(this);
			if (!uvAsync) return -1;
			if (int r = uv_async_init(loop, uvAsync, [](uv_async_t* handle) {
				UvGetSelf<UvAsync>(handle)->Execute();
			})) {
				uvAsync = nullptr;
				return r;
			}
			return 0;
		}
		int Dispatch(std::function<void()>&& action) noexcept {
			if (!uvAsync) return -1;
			{
				std::scoped_lock<std::mutex> g(mtx);
				actions.push_back(std::move(action));
			}
			return uv_async_send(uvAsync);
		}
		inline void Execute() noexcept {
			{
				std::scoped_lock<std::mutex> g(mtx);
				action = std::move(actions.front());
				actions.pop_front();
			}
			action();
		}
	};

	struct UvTimer {
		uv_timer_t* uvTimer = nullptr;
		uint64_t timeoutMS = 0;
		std::function<void()> OnFire;

		UvTimer() = default;
		UvTimer(UvTimer const&) = delete;
		UvTimer& operator=(UvTimer const&) = delete;

		virtual ~UvTimer() {
			UvHandleCloseAndFree(uvTimer);
		}
		inline int Init(uv_loop_t* const& loop) noexcept {
			if (uvTimer) return 0;
			uvTimer = UvAlloc<uv_timer_t>(this);
			if (!uvTimer) return -1;
			if (int r = uv_timer_init(loop, uvTimer)) {
				uvTimer = nullptr;
				return r;
			}
			return 0;
		}
		inline int Start(uint64_t const& timeoutMS, uint64_t const& repeatIntervalMS) noexcept {
			if (!uvTimer) return -1;
			this->timeoutMS = timeoutMS;
			return uv_timer_start(uvTimer, [](uv_timer_t* t) {
				auto self = UvGetSelf<UvTimer>(t);
				if (self->OnFire) {
					self->OnFire();
				}
			}, timeoutMS, repeatIntervalMS);
		}
		inline int Restart(uint64_t const& timeoutMS = 0) noexcept {
			if (!uvTimer) return -1;
			return uv_timer_start(uvTimer, uvTimer->timer_cb, timeoutMS ? timeoutMS : this->timeoutMS, 0);
		}
		inline int Stop() noexcept {
			if (!uvTimer) return -1;
			return uv_timer_stop(uvTimer);
		}
		inline int Again() noexcept {
			if (!uvTimer) return -1;
			return uv_timer_again(uvTimer);
		}
	};

	struct UvResolver;
	struct uv_getaddrinfo_t_ex {
		uv_getaddrinfo_t req;
		std::weak_ptr<UvResolver> resolver_w;
	};

	struct UvResolver : std::enable_shared_from_this<UvResolver> {
		UvLoop& loop;
		uv_getaddrinfo_t_ex* req = nullptr;
		UvTimer_s timeouter;
		std::vector<std::string> ips;
		std::function<void()> OnFinish;
		std::function<void()> OnTimeout;
#ifdef __IPHONE_OS_VERSION_MIN_REQUIRED
		addrinfo hints;
#endif

		UvResolver(UvLoop& loop) noexcept
			: loop(loop) {
#ifdef __IPHONE_OS_VERSION_MIN_REQUIRED
			hints.ai_family = PF_UNSPEC;
			hints.ai_socktype = SOCK_STREAM;
			hints.ai_protocol = 0;// IPPROTO_TCP;
			hints.ai_flags = AI_DEFAULT;
#endif
		}

		UvResolver(UvResolver const&) = delete;
		UvResolver& operator=(UvResolver const&) = delete;

		virtual ~UvResolver() {
			Cleanup();
		}

		inline void Cleanup() {
			ips.clear();
			timeouter.reset();
			if (req) {
				uv_cancel((uv_req_t*)req);
				req = nullptr;
			}
		}

		inline int SetTimeout(uint64_t const& timeoutMS = 0) {
			if (!timeoutMS) return 0;
			timeouter = CreateUvTimer<UvTimer>(loop, timeoutMS, 0, [self_w = std::weak_ptr<UvResolver>(shared_from_this())]{
				if (auto self = self_w.lock()) {
					self->Cleanup();
					if (self->OnTimeout) {
						self->OnTimeout();
					}
				}
				});
			return timeouter ? 0 : -1;
		}

		inline int Resolve(std::string const& domainName, uint64_t const& timeoutMS = 0) noexcept {
			if (int r = SetTimeout(timeoutMS)) return r;

			auto req = std::make_unique<uv_getaddrinfo_t_ex>();
			req->resolver_w = shared_from_this();
			if (int r = uv_getaddrinfo((uv_loop_t*)&loop.uvLoop, (uv_getaddrinfo_t*)&req->req, [](uv_getaddrinfo_t* req_, int status, struct addrinfo* ai) {
				auto req = std::unique_ptr<uv_getaddrinfo_t_ex>(container_of(req_, uv_getaddrinfo_t_ex, req));
				auto resolver = req->resolver_w.lock();
				if (!resolver) return;
				if (status) return;													// error or -4081 canceled
				assert(ai);

				auto& ips = resolver->ips;
				std::string s;
				do {
					s.resize(64);
					if (ai->ai_addr->sa_family == AF_INET6) {
						uv_ip6_name((sockaddr_in6*)ai->ai_addr, s.data(), s.size());
					}
					else {
						uv_ip4_name((sockaddr_in*)ai->ai_addr, s.data(), s.size());
					}
					s.resize(strlen(s.data()));

					if (std::find(ips.begin(), ips.end(), s) == ips.end()) {
						ips.push_back(std::move(s));								// known issue: ios || android will receive duplicate result
					}
					ai = ai->ai_next;
				} while (ai);
				uv_freeaddrinfo(ai);

				resolver->timeouter.reset();
				resolver->Finish();

			}, domainName.c_str(), nullptr,
#ifdef __IPHONE_OS_VERSION_MIN_REQUIRED
			(const addrinfo*)hints
#else
				nullptr
#endif
				)) return r;
			this->req = req.release();
			return 0;
		}

		inline virtual void Finish() noexcept {
			if (OnFinish) {
				OnFinish();
			}
		}
	};

	struct UvTcp : std::enable_shared_from_this<UvTcp> {
		uv_tcp_t* uvTcp = nullptr;

		inline bool Disposed() noexcept {
			return uvTcp == nullptr;
		}

		inline int Init(uv_loop_t* const& loop) noexcept {
			if (uvTcp) return 0;
			uvTcp = UvAlloc<uv_tcp_t>(this);
			if (!uvTcp) return -1;
			if (int r = uv_tcp_init(loop, uvTcp)) {
				uvTcp = nullptr;
				return r;
			}
			return 0;
		}
		virtual ~UvTcp() {
			UvHandleCloseAndFree(uvTcp);
		}
	};

	struct uv_write_t_ex : uv_write_t {
		uv_buf_t buf;
	};

	struct UvTcpBasePeer : UvTcp {
		Buffer buf;
		std::function<void()> OnDisconnect;

		UvTcpBasePeer() = default;
		UvTcpBasePeer(UvTcpBasePeer const&) = delete;
		UvTcpBasePeer& operator=(UvTcpBasePeer const&) = delete;

		inline virtual void Dispose(bool callback = true) noexcept {
			if (!uvTcp) return;
			UvHandleCloseAndFree(uvTcp);
			if (callback && OnDisconnect) {
				OnDisconnect();
				OnDisconnect = nullptr;		// 如果 peer 被该 func 持有, 可能导致 peer 内存释放
			}
		}

		inline int ReadStart() noexcept {
			if (!uvTcp) return -1;
			return uv_read_start((uv_stream_t*)uvTcp, UvAllocCB, [](uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
				auto self = UvGetSelf<UvTcpBasePeer>(stream);
				if (nread > 0) {
					nread = self->Unpack((uint8_t*)buf->base, (uint32_t)nread);
				}
				if(buf) ::free(buf->base);
				if (nread < 0) {
					self->Dispose();	// call OnDisconnect
				}
			});
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

		// reqbuf = uv_write_t_ex space + len space + data
		// len = data's len
		inline int SendReqPack(uint8_t* const& reqbuf, uint32_t const& len) {
			reqbuf[sizeof(uv_write_t_ex) + 0] = uint8_t(len);		// fill package len
			reqbuf[sizeof(uv_write_t_ex) + 1] = uint8_t(len >> 8);
			reqbuf[sizeof(uv_write_t_ex) + 2] = uint8_t(len >> 16);
			reqbuf[sizeof(uv_write_t_ex) + 3] = uint8_t(len >> 24);

			auto req = (uv_write_t_ex*)reqbuf;						// fill req args
			req->buf.base = (char*)(req + 1);
			req->buf.len = decltype(uv_buf_t::len)(len + 4);
			return Send(req);
		}

		inline int Send(uv_write_t_ex* const& req) noexcept {
			if (!uvTcp) return -1;
			// todo: check send queue len ? protect?  uv_stream_get_write_queue_size((uv_stream_t*)uvTcp);
			int r = uv_write(req, (uv_stream_t*)uvTcp, &req->buf, 1, [](uv_write_t *req, int status) {
				::free(req);
			});
			if (r) Dispose(false);	// do not call OnDisconnect
			return r;
		}

		inline int Send(uint8_t const* const& buf, ssize_t const& dataLen) noexcept {
			if (!uvTcp) return -1;
			auto req = (uv_write_t_ex*)::malloc(sizeof(uv_write_t_ex) + dataLen);
			memcpy(req + 1, buf, dataLen);
			req->buf.base = (char*)(req + 1);
			req->buf.len = decltype(uv_buf_t::len)(dataLen);
			return Send(req);
		}
	};

	// pack struct: [addr,] serial, data
	struct UvTcpPeer : UvTcpBasePeer {
		BBuffer recvBB;			// for replace buf memory decode package
		BBuffer sendBB;
		std::function<int(Object_s&& msg)> OnReceivePush;
		std::function<int(int const& serial, Object_s&& msg)> OnReceiveRequest;
		std::unordered_map<int, std::pair<std::function<int(Object_s&& msg)>, UvTimer_s>> callbacks;
		int serial = 0;

		UvTcpPeer() = default;
		UvTcpPeer(UvTcpPeer const&) = delete;
		UvTcpPeer& operator=(UvTcpPeer const&) = delete;

		inline virtual void Dispose(bool callback = true) noexcept override {
			if (!uvTcp) return;
			for (decltype(auto) kv : callbacks) {
				kv.second.first(nullptr);
			}
			callbacks.clear();
			auto self = shared_from_this();		// hold memory
			OnReceivePush = nullptr;
			OnReceiveRequest = nullptr;
			this->UvTcpBasePeer::Dispose(callback);
		}

		virtual int HandlePack(uint8_t* const& recvBuf, uint32_t const& recvLen) noexcept {
			recvBB.buf = (uint8_t*)recvBuf;
			xx::ScopeGuard sgBuf([&] {recvBB.buf = nullptr; });	// restore buf at func exit
			recvBB.len = recvLen;
			recvBB.cap = recvLen;
			recvBB.offset = 0;

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

		// serial == 0: push    > 0: response    < 0: request
		inline int SendPackage(Object_s const& data, int32_t const& serial = 0, int const& addr = 0) {
			if (Disposed()) return -1;
			sendBB.Reserve(sizeof(uv_write_t_ex) + 4);				// skip uv_write_t_ex + header space
			sendBB.len = sizeof(uv_write_t_ex) + 4;
			if (addr) sendBB.WriteFixed(addr);
			sendBB.Write(serial);
			sendBB.WriteRoot(data);

			auto buf = sendBB.buf;									// cut buf memory for send
			auto len = sendBB.len - sizeof(uv_write_t_ex) - 4;
			sendBB.buf = nullptr;
			sendBB.len = 0;
			sendBB.cap = 0;

			return SendReqPack(buf, (uint32_t)len);
		}

		inline int SendPush(Object_s const& data, int const& addr = 0) {
			return SendPackage(data);
		}

		inline int SendResponse(int32_t const& serial, Object_s const& data, int const& addr = 0) {
			return SendPackage(data, serial, addr);
		}

		inline int SendRequest(Object_s const& data, int const& addr, std::function<int(Object_s&& msg)>&& cb, uint64_t const& timeoutMS = 0) {
			if (Disposed()) return -1;
			std::pair<std::function<int(Object_s&& msg)>, UvTimer_s> v;
			++serial;
			if (timeoutMS) {
				v.second = CreateUvTimer(*container_of(uvTcp->loop, UvLoop, uvLoop), timeoutMS, 0, [this, serial = this->serial]() {
					TimeoutCallback(serial);
				});
				if (!v.second) return -1;
			}
			if (int r = SendPackage(data, -serial, addr)) return r;
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

	template<typename PeerType>
	struct UvTcpListener : UvTcp {
		std::function<std::shared_ptr<PeerType>()> OnCreatePeer;
		std::function<void(std::shared_ptr<PeerType>&& peer)> OnAccept;

		UvTcpListener() = default;
		UvTcpListener(UvTcpListener const&) = delete;
		UvTcpListener& operator=(UvTcpListener const&) = delete;

		inline virtual std::shared_ptr<PeerType> CreatePeer() noexcept {
			return OnCreatePeer ? OnCreatePeer() : std::make_shared<PeerType>();
		}
		inline virtual void Accept(std::shared_ptr<PeerType>&& peer) noexcept {
			if (OnAccept) {
				OnAccept(std::move(peer));
			}
		};
	};

	template<typename ListenerType = UvTcpListener<>>
	inline std::shared_ptr<ListenerType> CreateUvTcpListener(UvLoop& loop, std::string const& ip, int const& port, int const& backlog = 128) noexcept {
		auto listener = std::make_shared<ListenerType>();
		if (listener->Init(&loop.uvLoop)) return nullptr;

		sockaddr_in6 addr;
		if (ip.find(':') == std::string::npos) {								// ipv4
			if (uv_ip4_addr(ip.c_str(), port, (sockaddr_in*)&addr)) return nullptr;
		}
		else {																	// ipv6
			if (uv_ip6_addr(ip.c_str(), port, &addr)) return nullptr;
		}
		if (uv_tcp_bind(listener->uvTcp, (sockaddr*)&addr, 0)) return nullptr;

		if (uv_listen((uv_stream_t*)listener->uvTcp, backlog, [](uv_stream_t* server, int status) {
			if (status) return;
			auto self = UvGetSelf<ListenerType>(server);
			auto peer = self->CreatePeer();
			if (!peer || peer->Init(server->loop)) return;
			if (uv_accept(server, (uv_stream_t*)peer->uvTcp)) return;
			if (peer->ReadStart()) return;
			self->Accept(std::move(peer));
		})) return nullptr;

		return listener;
	}

	template<typename PeerType = UvTcpPeer>
	struct UvTcpDialer;

	template<typename PeerType>
	struct uv_connect_t_ex {
		uv_connect_t req;
		std::shared_ptr<PeerType> peer;	// temp holder
		std::weak_ptr<UvTcpDialer<PeerType>> client_w;	// weak ref
		int serial;
		int batchNumber;
		~uv_connect_t_ex();
	};

	template<typename PeerType>
	struct UvTcpDialer : std::enable_shared_from_this<UvTcpDialer<PeerType>> {
		using PeerType_s = std::shared_ptr<PeerType>;
		using ReqType = uv_connect_t_ex<PeerType>;
		UvLoop& loop;
		int serial = 0;
		std::unordered_map<int, ReqType*> reqs;
		int batchNumber = 0;
		UvTimer_s timeouter;		// singleton holder
		PeerType_s peer;	// singleton holder

		std::function<PeerType_s()> OnCreatePeer;
		std::function<void()> OnConnect;
		std::function<void()> OnTimeout;

		UvTcpDialer(UvLoop& loop) noexcept : loop(loop) {}
		UvTcpDialer(UvTcpDialer const&) = delete;
		UvTcpDialer& operator=(UvTcpDialer const&) = delete;

		virtual ~UvTcpDialer() {
			Cleanup();
		}

		// 0: free    1: dialing    2: connected
		inline int State() const noexcept {
			if (peer && !peer->Disposed()) return 2;
			if (reqs.size()) return 1;
			return 0;
		}

		inline void Cleanup(bool resetPeer = true) noexcept {
			timeouter.reset();
			if (resetPeer) {
				peer.reset();
			}
			for (decltype(auto) kv : reqs) {
				uv_cancel((uv_req_t*)kv.second);
			}
			reqs.clear();
			serial = 0;
			++batchNumber;
		}

		inline int SetTimeout(uint64_t const& timeoutMS = 0) noexcept {
			if (!timeoutMS) return 0;
			timeouter = CreateUvTimer<UvTimer>(loop, timeoutMS, 0, [self_w = std::weak_ptr<UvTcpDialer>(shared_from_this())]{
				if (auto self = self_w.lock()) {
					self->Cleanup(false);
					if (!self->peer && self->OnTimeout) {
						self->OnTimeout();
					}
				}
				});
			return timeouter ? 0 : -1;
		}

		inline int Dial(std::string const& ip, int const& port, uint64_t const& timeoutMS = 0, bool cleanup = true) noexcept {
			if (cleanup) {
				Cleanup();
			}

			sockaddr_in6 addr;
			if (ip.find(':') == std::string::npos) {								// ipv4
				if (int r = uv_ip4_addr(ip.c_str(), port, (sockaddr_in*)&addr)) return r;
			}
			else {																	// ipv6
				if (int r = uv_ip6_addr(ip.c_str(), port, &addr)) return r;
			}

			if (int r = SetTimeout(timeoutMS)) return r;

			auto req = std::make_unique<ReqType>();
			req->peer = CreatePeer();
			if (req->peer->Init(&loop.uvLoop)) return -2;

			req->client_w = shared_from_this();
			req->serial = ++serial;
			req->batchNumber = batchNumber;

			if (uv_tcp_connect(&req->req, req->peer->uvTcp, (sockaddr*)&addr, [](uv_connect_t* conn, int status) {
				auto req = std::unique_ptr<ReqType>(container_of(conn, ReqType, req));
				auto client = req->client_w.lock();
				if (!client) return;
				if (status) return;													// error or -4081 canceled
				if (client->batchNumber > req->batchNumber) return;
				if (client->peer) return;											// only fastest connected peer can survival

				if (req->peer->ReadStart()) return;
				client->peer = std::move(req->peer);								// connect success
				client->timeouter.reset();
				client->Connect();
			})) return -3;

			reqs[serial] = req.release();
			return 0;
		}

		inline int Dial(std::vector<std::string> const& ips, int const& port, uint64_t const& timeoutMS = 0) noexcept {
			Cleanup();
			if (int r = SetTimeout(timeoutMS)) return r;
			for (decltype(auto) ip : ips) {
				Dial(ip, port, 0, false);
			}
		}
		inline int Dial(std::vector<std::pair<std::string, int>> const& ipports, uint64_t const& timeoutMS = 0) noexcept {
			Cleanup();
			if (int r = SetTimeout(timeoutMS)) return r;
			for (decltype(auto) ipport : ipports) {
				Dial(ipport.first, ipport.second, 0, false);
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
	};

	template<typename PeerType>
	inline uv_connect_t_ex<PeerType>::~uv_connect_t_ex() {
		if (auto client = client_w.lock()) {
			client->reqs.erase(serial);
		}
	}

	inline UvAsync_s CreateUvAsync(UvLoop& loop) noexcept {
		auto async = std::make_shared<UvAsync>();
		if (async->Init(&loop.uvLoop)) return nullptr;
		return async;
	}

	template<typename TimerType, typename ENABLED>
	inline std::shared_ptr<TimerType> CreateUvTimer(UvLoop& loop, uint64_t const& timeoutMS, uint64_t const& repeatIntervalMS, std::function<void()>&& onFire) noexcept {
		auto timer = std::make_shared<TimerType>();
		if (timer->Init(&loop.uvLoop)) return nullptr;
		if (onFire) {
			timer->OnFire = std::move(onFire);
		}
		if (timer->Start(timeoutMS, repeatIntervalMS)) return nullptr;
		return timer;
	}
}
