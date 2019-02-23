#pragma once
#include "uv.h"
#include "xx.h"
#include <stdint.h>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <functional>
#include <cassert>
#include <mutex>

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

struct UvTimer;
struct UvResolver;
struct UvAsync;

struct UvLoop {
	uv_loop_t uvLoop;
	xx::MemPool mp;
	UvLoop() {
		if (int r = uv_loop_init(&uvLoop)) throw r;
	}
	UvLoop(UvLoop const&) = delete;
	UvLoop& operator=(UvLoop const&) = delete;

	inline int Run(uv_run_mode const& mode = UV_RUN_DEFAULT) noexcept {
		return uv_run(&uvLoop, mode);
	}

	template<typename ListenerType>
	xx::Unique<ListenerType> CreateListener(xx::String const& ip, int const& port, int const& backlog = 128) noexcept;

	template<typename ClientType>
	xx::Unique<ClientType> CreateClient() noexcept;

	xx::Unique<UvResolver> CreateResolver() noexcept;

	xx::Unique<UvAsync> CreateAsync() noexcept;

	template<typename TimerType = UvTimer>
	xx::Unique<TimerType> CreateTimer(uint64_t const& timeoutMS, uint64_t const& repeatIntervalMS, std::function<void()>&& onFire = nullptr) noexcept;
};

struct UvAsync {
	uv_async_t* uvAsync = nullptr;
	std::mutex mtx;
	xx::Queue<std::function<void()>> actions;
	std::function<void()> action;

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
			actions.Push(std::move(action));
		}
		return uv_async_send(uvAsync);
	}
	inline void Execute() noexcept {
		{
			std::scoped_lock<std::mutex> g(mtx);
			action = std::move(actions.Top());
			actions.Pop();
		}
		action();
	}
};

struct UvTimer {
	uv_timer_t* uvTimer = nullptr;
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
		return uv_timer_start(uvTimer, [](uv_timer_t* t) {
			auto self = UvGetSelf<UvTimer>(t);
			if (self->OnFire) {
				self->OnFire();
			}
		}, timeoutMS, repeatIntervalMS);
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
	xx::Weak<UvResolver> resolver_w;
};

struct UvResolver {
	UvLoop& loop;
	uv_getaddrinfo_t_ex* req = nullptr;
	xx::Unique<UvTimer> timeouter;
	xx::List<xx::String> ips;
	std::function<void()> OnFinish;
	std::function<void()> OnTimeout;
#ifdef __IPHONE_OS_VERSION_MIN_REQUIRED
	addrinfo hints;
#endif

	UvResolver(UvLoop& loop) noexcept
		: loop(loop)
		, ips(&loop.mp)
	{
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
		ips.Clear();
		timeouter.Reset();
		if (req) {
			uv_cancel((uv_req_t*)req);
			req = nullptr;
		}
	}

	inline int SetTimeout(uint64_t const& timeoutMS = 0) {
		if (!timeoutMS) return 0;
		timeouter = loop.CreateTimer<UvTimer>(timeoutMS, 0, [self_w = xx::Weak<UvResolver>(this)] {
			if (self_w) {
				auto self = self_w.pointer;
				self->Cleanup();
				if (self->OnTimeout) {
					self->OnTimeout();
				}
			}
		});
		return timeouter ? 0 : -1;
	}

	inline int Resolve(xx::String const& domainName, uint64_t const& timeoutMS = 0) noexcept {
		if (int r = SetTimeout(timeoutMS)) return r;

		auto req = std::make_unique<uv_getaddrinfo_t_ex>();
		req->resolver_w = this;
		if (int r = uv_getaddrinfo((uv_loop_t*)&loop.uvLoop, (uv_getaddrinfo_t*)&req->req, [](uv_getaddrinfo_t* req_, int status, struct addrinfo* ai) {
			auto req = std::unique_ptr<uv_getaddrinfo_t_ex>(container_of(req_, uv_getaddrinfo_t_ex, req));
			if (!req->resolver_w) return;
			if (status) return;													// error or -4081 canceled
			auto resolver = req->resolver_w.pointer;
			assert(ai);

			auto& ips = resolver->ips;
			xx::String s(&resolver->loop.mp);
			do {
				s.Resize(64);
				if (ai->ai_addr->sa_family == AF_INET6) {
					uv_ip6_name((sockaddr_in6*)ai->ai_addr, s.buf, s.bufLen);
				}
				else {
					uv_ip4_name((sockaddr_in*)ai->ai_addr, s.buf, s.bufLen);
				}
				s.Resize(strlen(s.buf));

				if (ips.Find(s) == size_t(-1))
					ips.Add(std::move(s));								// known issue: ios || android will receive duplicate result
				}
				ai = ai->ai_next;
			} while (ai);
			uv_freeaddrinfo(ai);

			resolver->timeouter.Reset();
			resolver->Finish();

		}, ((xx::String&)domainName).c_str(), nullptr,
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

struct UvTcp {
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

struct UvTcpBasePeer;
struct uv_write_t_ex : uv_write_t {
	uv_buf_t buf;
};

struct UvTcpBasePeer : UvTcp {
	std::function<int(uint8_t const* const& buf, uint32_t const& len)> OnReceive;
	std::function<void()> OnDisconnect;

	UvTcpBasePeer() = default;
	UvTcpBasePeer(UvTcpBasePeer const&) = delete;
	UvTcpBasePeer& operator=(UvTcpBasePeer const&) = delete;

	inline virtual void Dispose(bool callback = true) noexcept {
		if (!uvTcp) return;
		UvHandleCloseAndFree(uvTcp);
		if (callback && OnDisconnect) {
			OnDisconnect();
		}
	}

	inline int ReadStart() noexcept {
		if (!uvTcp) return -1;
		return uv_read_start((uv_stream_t*)uvTcp, [](uv_handle_t* h, size_t suggested_size, uv_buf_t* buf) {
			buf->base = (char*)malloc(suggested_size);
			buf->len = decltype(uv_buf_t::len)(suggested_size);
		}, [](uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
			auto self = UvGetSelf<UvTcpBasePeer>(stream);
			if (nread > 0) {
				nread = self->Unpack((uint8_t*)buf->base, (uint32_t)nread);
			}
			free(buf->base);
			if (nread < 0) {
				self->Dispose();	// call OnDisconnect
			}
		});
	}

	inline int Send(uv_write_t_ex* const& req) noexcept {
		if (!uvTcp) return -1;
		// todo: check send queue len ? protect?  uv_stream_get_write_queue_size((uv_stream_t*)uvTcp);
		int r = uv_write(req, (uv_stream_t*)uvTcp, &req->buf, 1, [](uv_write_t *req, int status) {
			free(req);
		});
		if (r) Dispose(false);	// do not call OnDisconnect
		return r;
	}

	inline int Send(uint8_t const* const& buf, ssize_t const& dataLen) noexcept {
		if (!uvTcp) return -1;
		auto req = (uv_write_t_ex*)malloc(sizeof(uv_write_t_ex) + dataLen);
		memcpy(req + 1, buf, dataLen);
		req->buf.base = (char*)(req + 1);
		req->buf.len = decltype(uv_buf_t::len)(dataLen);
		return Send(req);
	}

	virtual int Unpack(uint8_t const* const& buf, uint32_t const& len) noexcept {
		return OnReceive ? OnReceive(buf, len) : 0;
	}
};

struct UvTcpBaseListener : UvTcp {
	std::function<xx::Unique<UvTcpBasePeer>()> OnCreatePeer;
	std::function<void(xx::Unique<UvTcpBasePeer>&& peer)> OnAccept;

	UvTcpBaseListener() = default;
	UvTcpBaseListener(UvTcpBaseListener const&) = delete;
	UvTcpBaseListener& operator=(UvTcpBaseListener const&) = delete;

	inline virtual xx::Unique<UvTcpBasePeer> CreatePeer() noexcept {
		return OnCreatePeer ? OnCreatePeer() : std::make_shared<UvTcpBasePeer>();
	}
	inline virtual void Accept(xx::Unique<UvTcpBasePeer>&& peer) noexcept {
		if (OnAccept) {
			OnAccept(std::move(peer));
		}
	};
};

template<typename ListenerType>
inline xx::Unique<ListenerType> UvLoop::CreateListener(xx::String const& ip, int const& port, int const& backlog) noexcept {
	auto listener = std::make_shared<ListenerType>();
	if (listener->Init(&uvLoop)) return nullptr;

	sockaddr_in6 addr;
	if (ip.find(':') == xx::String::npos) {								// ipv4
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

struct UvTcpBaseClient;
struct uv_connect_t_ex {
	uv_connect_t req;
	xx::Unique<UvTcpBasePeer> peer;		// temp holder
	xx::Weak<UvTcpBaseClient> client_w;	// weak ref
	int serial;
	int batchNumber;
	~uv_connect_t_ex();
};

struct UvTcpBaseClient {
	UvLoop& loop;
	int serial = 0;
	std::unordered_map<int, uv_connect_t_ex*> reqs;
	int batchNumber = 0;
	xx::Unique<UvTimer> timeouter;		// singleton holder
	xx::Unique<UvTcpBasePeer> peer;	// singleton holder

	std::function<xx::Unique<UvTcpBasePeer>()> OnCreatePeer;
	std::function<void()> OnConnect;
	std::function<void()> OnTimeout;

	UvTcpBaseClient(UvLoop& loop) noexcept : loop(loop) {}
	UvTcpBaseClient(UvTcpBaseClient const&) = delete;
	UvTcpBaseClient& operator=(UvTcpBaseClient const&) = delete;

	virtual ~UvTcpBaseClient() {
		Cleanup();
	}

	inline void Cleanup(bool resetPeer = true) {
		timeouter.Reset();
		if (resetPeer) {
			peer.Reset();
		}
		for (decltype(auto) kv : reqs) {
			uv_cancel((uv_req_t*)kv.second);
		}
		reqs.clear();
		++batchNumber;
	}

	inline int SetTimeout(uint64_t const& timeoutMS = 0) {
		if (!timeoutMS) return 0;
		timeouter = loop.CreateTimer<UvTimer>(timeoutMS, 0, [self_w = xx::Weak<UvTcpBaseClient>(this)]{
			if (auto self = self_w.Lock()) {
				self->Cleanup(false);
				if (!self->peer && self->OnTimeout) {
					self->OnTimeout();
				}
			}
			});
		return timeouter ? 0 : -1;
	}

	inline int Dial(xx::String const& ip, int const& port, uint64_t const& timeoutMS = 0, bool cleanup = true) noexcept {
		if (cleanup) {
			Cleanup();
		}

		sockaddr_in6 addr;
		if (ip.Find(':') == size_t(-1)) {										// ipv4
			if (int r = uv_ip4_addr(((xx::String&)ip).c_str(), port, (sockaddr_in*)&addr)) return r;
		}
		else {																	// ipv6
			if (int r = uv_ip6_addr(((xx::String&)ip).c_str(), port, &addr)) return r;
		}

		if (int r = SetTimeout(timeoutMS)) return r;

		auto req = std::make_unique<uv_connect_t_ex>();
		req->peer = CreatePeer();
		if (req->peer->Init(&loop.uvLoop)) return -2;

		req->client_w = xx::Weak<UvTcpBaseClient>(this)();
		req->serial = ++serial;
		req->batchNumber = batchNumber;

		if (uv_tcp_connect(&req->req, req->peer->uvTcp, (sockaddr*)&addr, [](uv_connect_t* conn, int status) {
			auto req = std::unique_ptr<uv_connect_t_ex>(container_of(conn, uv_connect_t_ex, req));
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

	inline int Dial(xx::List<xx::String> const& ips, int const& port, uint64_t const& timeoutMS = 0) noexcept {
		Cleanup();
		if (int r = SetTimeout(timeoutMS)) return r;
		for (decltype(auto) ip : ips) {
			Dial(ip, port, 0, false);
		}
	}
	inline int Dial(xx::List<std::pair<xx::String, int>> const& ipports, uint64_t const& timeoutMS = 0) noexcept {
		Cleanup();
		if (int r = SetTimeout(timeoutMS)) return r;
		for (decltype(auto) ipport : ipports) {
			Dial(ipport.first, ipport.second, 0, false);
		}
	}

	inline virtual xx::Unique<UvTcpBasePeer> CreatePeer() noexcept {
		return std::make_shared<UvTcpBasePeer>();
	}
	inline virtual void Connect() noexcept {
		if (OnConnect) {
			OnConnect();
		}
	}
};

inline uv_connect_t_ex::~uv_connect_t_ex() {
	if (auto client = client_w.lock()) {
		client->reqs.erase(serial);
	}
}

template<typename ClientType>
inline xx::Unique<ClientType> UvLoop::CreateClient() noexcept {
	return std::make_shared<ClientType>(*this);
}

inline xx::Unique<UvResolver> UvLoop::CreateResolver() noexcept {
	return std::make_shared<UvResolver>(*this);
}

inline xx::Unique<UvAsync> UvLoop::CreateAsync() noexcept {
	auto async = std::make_shared<UvAsync>();
	if (async->Init(&uvLoop)) return nullptr;
	return async;
}

template<typename TimerType>
inline xx::Unique<TimerType> UvLoop::CreateTimer(uint64_t const& timeoutMS, uint64_t const& repeatIntervalMS, std::function<void()>&& onFire) noexcept {
	auto timer = std::make_shared<TimerType>();
	if (timer->Init(&uvLoop)) return nullptr;
	if (onFire) {
		timer->OnFire = std::move(onFire);
	}
	if (timer->Start(timeoutMS, repeatIntervalMS)) return nullptr;
	return timer;
}
