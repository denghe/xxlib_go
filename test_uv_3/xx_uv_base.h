#pragma once
#include "uv.h"
#include <stdint.h>
#include <cstdlib>
#include <cstring>
#include <string>
#include <memory>
#include <unordered_map>
#include <functional>
#include <cassert>
#include <vector>

#ifndef _offsetof
#define _offsetof(s,m) ((size_t)&reinterpret_cast<char const volatile&>((((s*)0)->m)))
#endif

#ifndef container_of
#define container_of(ptr, type, member) \
  ((type *) ((char *) (ptr) - _offsetof(type, member)))
#endif


// 重要：
// std::function 的捕获列表不可以随意增加引用以导致无法析构, 尽量使用 weak_ptr. 除非该对象仅依附于 std::function
// 如果在析构时会触发回调，派生类需要在自己的析构中 callback 以避免派生类被析构之后在基类中触发回调从而访问到已析构成员

struct UvItem {
	bool inited = false;
	bool disposed = false;
	void Dispose(uv_handle_t* const& h) noexcept {
		if (disposed) return;
		disposed = true;
		if (!inited) return;
		assert(!uv_is_closing(h));
		uv_close(h, nullptr);
	}
	virtual ~UvItem() = 0;	// Dispose((uv_handle_t*)&XXXXXXX);
};

struct UvTimer;

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

	template<typename ListenerType>
	std::shared_ptr<ListenerType> CreateListener(std::string const& ip, int const& port, int const& backlog = 128) noexcept;

	template<typename ClientType>
	std::shared_ptr<ClientType> CreateClient() noexcept;

	template<typename TimerType = UvTimer>
	std::shared_ptr<TimerType> CreateTimer(uint64_t const& timeoutMS, uint64_t const& repeatIntervalMS, std::function<void()>&& onFire = nullptr) noexcept;
};

// todo: async

struct UvTimer : UvItem {
	uv_timer_t uvTimer;
	std::function<void()> OnFire;

	~UvTimer() {
		Dispose((uv_handle_t*)&uvTimer);
	}
	inline int Init(uv_loop_t* const& loop) noexcept {
		if (inited) return 0;
		if (int r = uv_timer_init(loop, &uvTimer)) return r;
		inited = true;
		return 0;
	}
	inline int Start(uint64_t const& timeoutMS, uint64_t const& repeatIntervalMS) {
		if (disposed) return -1;
		return uv_timer_start(&uvTimer, [](uv_timer_t* t) {
			auto self = container_of(t, UvTimer, uvTimer);
			if (self->OnFire) {
				self->OnFire();
			}
		}, timeoutMS, repeatIntervalMS);
	}
	inline int Stop() {
		if (disposed) return -1;
		return uv_timer_stop(&uvTimer);
	}
	inline int Again() {
		if (disposed) return -1;
		return uv_timer_again(&uvTimer);
	}
};

struct UvTcp : UvItem {
	uv_tcp_t uvTcp;

	inline int Init(uv_loop_t* const& loop) noexcept {
		if (inited) return 0;
		if (int r = uv_tcp_init(loop, &uvTcp)) return r;
		inited = true;
		return 0;
	}
	~UvTcp() {
		Dispose((uv_handle_t*)&uvTcp);
	}
};

struct UvTcpPeerBase;
struct uv_write_t_ex : uv_write_t {
	uv_buf_t buf;
};

struct UvTcpPeerBase : UvTcp {
	std::function<int(uint8_t const* const& buf, uint32_t const& len)> OnReceive;
	std::function<void()> OnDisconnect;

	UvTcpPeerBase() = default;
	UvTcpPeerBase(UvTcpPeerBase const&) = delete;
	UvTcpPeerBase& operator=(UvTcpPeerBase const&) = delete;

	inline virtual void Dispose(bool callback = true) {
		if (disposed) return;
		if (callback && OnDisconnect) {
			OnDisconnect();
		}
		this->UvTcp::Dispose((uv_handle_t*)&uvTcp);
	}

	inline int ReadStart() noexcept {
		assert(!disposed);
		return uv_read_start((uv_stream_t*)&uvTcp, [](uv_handle_t* h, size_t suggested_size, uv_buf_t* buf) {
			buf->base = (char*)malloc(suggested_size);
			buf->len = decltype(uv_buf_t::len)(suggested_size);
		}, [](uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
			auto self = container_of(stream, UvTcpPeerBase, uvTcp);
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
		assert(!disposed);
		// todo: check send queue len ? protect?
		int r = uv_write(req, (uv_stream_t*)&uvTcp, &req->buf, 1, [](uv_write_t *req, int status) {
			free(req);
		});
		if (r) Dispose(false);	// do not call OnDisconnect
		return r;
	}

	inline int Send(char const* const& buf, ssize_t const& dataLen) noexcept {
		assert(!disposed);
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

struct UvTcpListenerBase : UvTcp {
	std::function<std::shared_ptr<UvTcpPeerBase>()> OnCreatePeer;
	std::function<void(std::shared_ptr<UvTcpPeerBase>&& peer)> OnAccept;

	UvTcpListenerBase() = default;
	UvTcpListenerBase(UvTcpListenerBase const&) = delete;
	UvTcpListenerBase& operator=(UvTcpListenerBase const&) = delete;

	inline virtual std::shared_ptr<UvTcpPeerBase> CreatePeer() noexcept {
		return OnCreatePeer ? OnCreatePeer() : std::make_shared<UvTcpPeerBase>();
	}
	inline virtual void Accept(std::shared_ptr<UvTcpPeerBase>&& peer) noexcept {
		if (OnAccept) {
			OnAccept(std::move(peer));
		}
	};
};

template<typename ListenerType>
inline std::shared_ptr<ListenerType> UvLoop::CreateListener(std::string const& ip, int const& port, int const& backlog) noexcept {
	auto listener = std::make_shared<ListenerType>();
	if (listener->Init(&uvLoop)) return nullptr;

	sockaddr_in6 addr;
	if (ip.find(':') == std::string::npos) {								// ipv4
		if (uv_ip4_addr(ip.c_str(), port, (sockaddr_in*)&addr)) return nullptr;
	}
	else {																	// ipv6
		if (uv_ip6_addr(ip.c_str(), port, &addr)) return nullptr;
	}
	if (uv_tcp_bind(&listener->uvTcp, (sockaddr*)&addr, 0)) return nullptr;

	if (uv_listen((uv_stream_t*)&listener->uvTcp, backlog, [](uv_stream_t* server, int status) {
		if (status) return;
		auto self = container_of(server, UvTcpListenerBase, uvTcp);
		auto peer = self->CreatePeer();
		if (!peer || peer->Init(server->loop)) return;
		if (uv_accept(server, (uv_stream_t*)&peer->uvTcp)) return;
		if (peer->ReadStart()) return;
		self->Accept(std::move(peer));
	})) return nullptr;

	return listener;
}

struct UvTcpClientBase;
struct uv_connect_t_ex {
	uv_connect_t req;
	std::shared_ptr<UvTcpPeerBase> peer;		// temp holder
	std::weak_ptr<UvTcpClientBase> client_w;	// weak ref
	int serial;
	int batchNumber;
	~uv_connect_t_ex();
};

struct UvTcpClientBase : UvItem, std::enable_shared_from_this<UvTcpClientBase> {
	UvLoop& loop;
	int serial = 0;
	std::unordered_map<int, uv_connect_t_ex*> reqs;
	int batchNumber = 0;
	std::shared_ptr<UvTcpPeerBase> peer;	// single holder

	std::function<std::shared_ptr<UvTcpPeerBase>()> OnCreatePeer;
	std::function<void()> OnConnected;

	UvTcpClientBase(UvLoop& loop) noexcept : loop(loop) {}
	UvTcpClientBase(UvTcpClientBase const&) = delete;
	UvTcpClientBase& operator=(UvTcpClientBase const&) = delete;

	~UvTcpClientBase() {
		Dispose();
	}
	inline virtual void Dispose(bool callback = false) noexcept {
		Cleanup(callback);
	}

	// return errNum or serial
	inline int Connect(std::string const& ip, int const& port, bool cleanup = true) noexcept {
		assert(!disposed);
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

		auto peer = CreatePeer();
		if (peer->Init(&loop.uvLoop)) return -1;

		auto req = new uv_connect_t_ex();
		req->peer = std::move(peer);
		req->client_w = shared_from_this();
		req->serial = ++serial;
		req->batchNumber = batchNumber;

		if (uv_tcp_connect(&req->req, &req->peer->uvTcp, (sockaddr*)&addr, [](uv_connect_t* conn, int status) {
			auto req = std::unique_ptr<uv_connect_t_ex>(container_of(conn, uv_connect_t_ex, req));
			auto client = req->client_w.lock();
			if (!client || client->disposed) return;
			if (status) return;													// error or -4081 canceled
			if (client->batchNumber > req->batchNumber) return;
			if (client->peer) return;											// only fastest connected peer can survival

			if (req->peer->ReadStart()) return;
			client->peer = std::move(req->peer);								// connect success
			if (client->OnConnected) {
				client->OnConnected();
			}
		})) {
			delete req;
			return -1;
		}
		else {
			reqs[serial] = req;
			return serial;
		}
	}

	inline void Cleanup(bool callback = false) {
		peer.reset();
		for (decltype(auto) kv : reqs) {
			uv_cancel((uv_req_t*)kv.second);
		}
		reqs.clear();
		++batchNumber;
	}

	inline int BatchConnect(std::vector<std::string> const& ips, int const& port) noexcept {
		Cleanup();
		for (decltype(auto) ip : ips) {
			Connect(ip, port, false);
		}
	}
	inline int BatchConnect(std::vector<std::pair<std::string, int>> const& ipports) noexcept {
		Cleanup();
		for (decltype(auto) ipport : ipports) {
			Connect(ipport.first, ipport.second, false);
		}
	}

	inline virtual std::shared_ptr<UvTcpPeerBase> CreatePeer() noexcept {
		return std::make_shared<UvTcpPeerBase>();
	}
	inline virtual void OnConnect(std::shared_ptr<UvTcpPeerBase> peer) noexcept {}
};

inline uv_connect_t_ex::~uv_connect_t_ex() {
	if (auto client = client_w.lock()) {
		client->reqs.erase(serial);
	}
}

template<typename ClientType>
inline std::shared_ptr<ClientType> UvLoop::CreateClient() noexcept {
	return std::make_shared<ClientType>(*this);
}

template<typename TimerType>
inline std::shared_ptr<TimerType> UvLoop::CreateTimer(uint64_t const& timeoutMS, uint64_t const& repeatIntervalMS, std::function<void()>&& onFire) noexcept {
	auto timer = std::make_shared<TimerType>();
	if (timer->Init(&uvLoop)) return nullptr;
	if (onFire) {
		timer->OnFire = std::move(onFire);
	}
	if (timer->Start(timeoutMS, repeatIntervalMS)) return nullptr;
	return timer;
}
