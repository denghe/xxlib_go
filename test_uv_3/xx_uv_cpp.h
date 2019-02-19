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

struct UvItem {
	int indexAtContainer = -1;
	inline bool Disposed() {
		return indexAtContainer == -1;
	}
	virtual void Dispose() noexcept = 0;
};

struct UvTimer;

struct UvLoop {
	uv_loop_t uvLoop;
	std::vector<std::shared_ptr<UvItem>> items;
	template<typename T>
	inline void ItemsPushBack(T&& item) {
		item->indexAtContainer = (int)items.size();
		items.push_back(std::forward<T>(item));
	}
	inline void ItemsSwapRemoveAt(int const& idx) {
		assert(idx >= 0 && idx < items.size());
		items[idx]->indexAtContainer = -1;
		auto last = items.size() - 1;
		if (idx < last) {
			items[last]->indexAtContainer = idx;
			items[idx] = std::move(items[last]);
		}
		items.pop_back();
	}

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

	std::shared_ptr<UvTimer> CreateTimer(uint64_t const& timeoutMS, uint64_t const& repeatIntervalMS, std::function<void()>&& onFire) noexcept;

	inline void Stop() noexcept {
		if (items.size()) {
			for (int i = (int)items.size() - 1; i >= 0; --i) {
				items[i]->Dispose();
			}
		}
	}

	~UvLoop() {
		int r = uv_loop_close(&uvLoop);
		assert(!r);
	}
};

// todo: async
struct UvTimer : UvItem {
	uv_timer_t uvTimer;
	std::function<void()> OnFire;
	inline int Init(uv_loop_t* const& loop) noexcept {
		return uv_timer_init(loop, &uvTimer);
	}
	inline virtual void Dispose() noexcept override {
		assert(!Disposed());
		auto h = (uv_handle_t*)&uvTimer;
		assert(!uv_is_closing(h));
		uv_close(h, [](uv_handle_t* h) {
			auto self = container_of(h, UvTimer, uvTimer);
			container_of(self->uvTimer.loop, UvLoop, uvLoop)->ItemsSwapRemoveAt(self->indexAtContainer);
		});
	}
	inline int Start(uint64_t const& timeoutMS, uint64_t const& repeatIntervalMS) {
		assert(!Disposed());
		return uv_timer_start(&uvTimer, [](uv_timer_t* t) {
			auto self = container_of(t, UvTimer, uvTimer);
			if (self->OnFire) {
				self->OnFire();
			}
		}, timeoutMS, repeatIntervalMS);
	}
	inline int Stop() {
		assert(!Disposed());
		return uv_timer_stop(&uvTimer);
	}
	inline int Again() {
		assert(!Disposed());
		return uv_timer_again(&uvTimer);
	}
};

struct UvTcp : UvItem {
	uv_tcp_t uvTcp;

	inline int Init(uv_loop_t* const& loop) noexcept {
		return uv_tcp_init(loop, &uvTcp);
	}
	inline virtual void Dispose() noexcept override {
		assert(!Disposed());
		auto h = (uv_handle_t*)&uvTcp;
		assert(!uv_is_closing(h));
		uv_close(h, [](uv_handle_t* h) {
			auto self = container_of(h, UvTcp, uvTcp);
			container_of(self->uvTcp.loop, UvLoop, uvLoop)->ItemsSwapRemoveAt(self->indexAtContainer);
		});
	}
};

struct UvTcpPeer : UvTcp {
	UvTcpPeer() = default;
	UvTcpPeer(UvTcpPeer const&) = delete;
	UvTcpPeer& operator=(UvTcpPeer const&) = delete;

	inline void ReadStart() noexcept {
		assert(!Disposed());
		uv_read_start((uv_stream_t*)&uvTcp, [](uv_handle_t* h, size_t suggested_size, uv_buf_t* buf) {
			buf->base = (char*)malloc(suggested_size);
			buf->len = decltype(uv_buf_t::len)(suggested_size);
		}, [](uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
			auto self = container_of(stream, UvTcpPeer, uvTcp);
			if (nread > 0) {
				nread = self->Unpack(buf->base, (uint32_t)nread);
			}
			free(buf->base);
			if (nread < 0) {
				self->Dispose();
			}
		});
	}
	inline virtual int Send(char const* const& buf, ssize_t const& dataLen) noexcept {
		assert(!Disposed());
		struct uv_write_t_ex : uv_write_t {
			uv_buf_t buf;
		};
		auto req = (uv_write_t_ex*)malloc(sizeof(uv_write_t_ex) + dataLen);
		memcpy(req + 1, buf, dataLen);
		req->buf.base = (char*)(req + 1);
		req->buf.len = decltype(uv_buf_t::len)(dataLen);

		return uv_write(req, (uv_stream_t*)&uvTcp, &req->buf, 1, [](uv_write_t *req, int status) {
			free(req);
			if (status) {
				container_of(req->handle, UvTcpPeer, uvTcp)->Dispose();
			}
		});
	}
	virtual int Unpack(char const* const& buf, uint32_t const& len) noexcept = 0;
};

struct UvTcpListener : UvTcp {
	UvTcpListener() = default;
	UvTcpListener(UvTcpListener const&) = delete;
	UvTcpListener& operator=(UvTcpListener const&) = delete;

	virtual std::shared_ptr<UvTcpPeer> OnCreatePeer() noexcept = 0;
	inline virtual void OnAccept(std::shared_ptr<UvTcpPeer> peer) noexcept {};
};

template<typename ListenerType>
inline std::shared_ptr<ListenerType> UvLoop::CreateListener(std::string const& ip, int const& port, int const& backlog) noexcept {
	auto listener = std::make_shared<ListenerType>();
	if (listener->Init(&uvLoop)) return nullptr;

	sockaddr_in6 addr;
	if (ip.find(':') == std::string::npos) {								// ipv4
		if (uv_ip4_addr(ip.c_str(), port, (sockaddr_in*)&addr)) goto LabEnd;
	}
	else {																	// ipv6
		if (uv_ip6_addr(ip.c_str(), port, &addr)) goto LabEnd;
	}
	if (uv_tcp_bind(&listener->uvTcp, (sockaddr*)&addr, 0)) goto LabEnd;

	if (uv_listen((uv_stream_t*)&listener->uvTcp, backlog, [](uv_stream_t* server, int status) {
		if (status) return;
		auto self = container_of(server, UvTcpListener, uvTcp);
		auto peer = self->OnCreatePeer();
		if (!peer || peer->Init(server->loop)) return;

		container_of(server->loop, UvLoop, uvLoop)->ItemsPushBack(peer);

		if (uv_accept(server, (uv_stream_t*)&peer->uvTcp)) {
			peer->Dispose();
			return;
		}
		peer->ReadStart();
		self->OnAccept(peer);
	})) goto LabEnd;

	ItemsPushBack(listener);
	return listener;
LabEnd:
	uv_close((uv_handle_t*)&listener->uvTcp, nullptr);
	return listener;
}

struct UvTcpClient;
struct uv_connect_t_ex : uv_connect_t {
	std::shared_ptr<UvTcpPeer> peer;
	std::shared_ptr<UvTcpClient> client;
	int serial;
	int batchNumber;
};

struct UvTcpClient : UvItem, std::enable_shared_from_this<UvTcpClient> {
	UvLoop& loop;
	int serial = 0;
	std::unordered_map<int, uv_connect_t_ex*> reqs;
	int batchNumber = 0;
	std::shared_ptr<UvTcpPeer> peer;

	UvTcpClient(UvLoop& loop) noexcept : loop(loop) {}
	UvTcpClient(UvTcpClient const&) = delete;
	UvTcpClient& operator=(UvTcpClient const&) = delete;

	inline virtual void Dispose() noexcept override {
		Cleanup();
		loop.ItemsSwapRemoveAt(indexAtContainer);
	}

	// return errNum or serial
	inline int Connect(std::string const& ip, int const& port, bool cleanup = true) noexcept {
		assert(!Disposed());
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

		auto peer = OnCreatePeer();
		if (peer->Init(&loop.uvLoop)) return -1;

		auto req = new uv_connect_t_ex();
		req->peer = std::move(peer);
		req->client = shared_from_this();
		req->serial = ++serial;
		req->batchNumber = batchNumber;

		if (uv_tcp_connect(req, &req->peer->uvTcp, (sockaddr*)&addr, [](uv_connect_t* req_, int status) {
			auto req = std::unique_ptr<uv_connect_t_ex, std::function<void(uv_connect_t_ex *)>>((uv_connect_t_ex*)req_, [](uv_connect_t_ex * req) {
				if (req->peer) {
					uv_close((uv_handle_t*)&req->peer->uvTcp, nullptr);
				}
				req->client->reqs.erase(req->serial);
				delete req;
			});
			if (req->client->Disposed()) return;
			if (status) return;													// error or -4081 canceled

			auto peer = std::move(req->peer);									// skip uv_close
			req->client->loop.ItemsPushBack(peer);
			peer->ReadStart();
			if (req->client->batchNumber == req->batchNumber && !req->client->peer) {
				req->client->peer = peer;
				req->client->OnConnect(std::move(peer));						// connect success
			}
		})) {
			uv_close((uv_handle_t*)&req->peer->uvTcp, nullptr);
			delete req;
			return -1;
		}
		else {
			reqs[serial] = req;
			return serial;
		}
	}

	inline void Cleanup() {
		if (peer) {
			if (!peer->Disposed()) {
				peer->Dispose();
			}
			peer.reset();
		}
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

	inline virtual void OnConnect(std::shared_ptr<UvTcpPeer> peer) noexcept {}
	virtual std::shared_ptr<UvTcpPeer> OnCreatePeer() noexcept = 0;
};

template<typename ClientType>
inline std::shared_ptr<ClientType> UvLoop::CreateClient() noexcept {
	auto client = std::make_shared<ClientType>(*this);
	ItemsPushBack(client);
	return client;
}

inline std::shared_ptr<UvTimer> UvLoop::CreateTimer(uint64_t const& timeoutMS, uint64_t const& repeatIntervalMS, std::function<void()>&& onFire = nullptr) noexcept {
	auto timer = std::make_shared<UvTimer>();
	if (timer->Init(&uvLoop)) return nullptr;
	ItemsPushBack(timer);
	if (onFire) {
		timer->OnFire = std::move(onFire);
	}
	if (timer->Start(timeoutMS, repeatIntervalMS)) {
		timer->Dispose();
		return nullptr;
	}
	return timer;
}
