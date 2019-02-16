#pragma once
#include "uv.h"
#include <stdint.h>
#include <cstdlib>
#include <cstring>
#include <string>
#include <memory>
#include <cassert>
#include <vector>

#ifndef _offsetof
#define _offsetof(s,m) ((size_t)&reinterpret_cast<char const volatile&>((((s*)0)->m)))
#endif

#ifndef container_of
#define container_of(ptr, type, member) \
  ((type *) ((char *) (ptr) - _offsetof(type, member)))
#endif

template<typename ContainerType>
inline void SwapRemoveAt(ContainerType& items, int const& idx) {
	assert(idx >= 0 && idx < items.size());
	auto last = items.size() - 1;
	if (idx < last) {
		items[last]->indexAtContainer = idx;
		items[idx] = std::move(items[last]);
	}
	items.pop_back();
}

struct UvItem {
	int indexAtContainer = -1;
};

struct UvLoop {
	uv_loop_t uvLoop;
	std::vector<std::shared_ptr<UvItem>> items;

	UvLoop() {
		if (int r = uv_loop_init(&uvLoop)) throw r;
	}
	UvLoop(UvLoop const&) = delete;
	UvLoop& operator=(UvLoop const&) = delete;

	inline int Run(uv_run_mode const& mode = UV_RUN_DEFAULT) noexcept {
		return uv_run(&uvLoop, mode);
	}

	template<typename ListenerType>
	inline std::weak_ptr<ListenerType> CreateListener(std::string const& ip, int const& port, int const& backlog = 128) noexcept;

	template<typename ClientType>
	inline std::weak_ptr<ClientType> CreateClient() noexcept;

	~UvLoop() {
		items.clear();
		int r = uv_loop_close(&uvLoop);
		assert(!r);
	}
};

struct UvTcp : UvItem {
	uv_tcp_t uvTcp;

	inline int Init(uv_loop_t* const& loop) noexcept {
		return uv_tcp_init(loop, &uvTcp);
	}
	inline virtual void Close(bool bySend = false) noexcept {
		auto h = (uv_handle_t*)&uvTcp;
		assert(!uv_is_closing(h));
		uv_close(h, [](uv_handle_t* h) {
			auto self = container_of(h, UvTcp, uvTcp);
			SwapRemoveAt(container_of(self->uvTcp.loop, UvLoop, uvLoop)->items, self->indexAtContainer);
		});
	}
};

struct UvTcpPeer : UvTcp {
	UvTcpPeer() = default;
	UvTcpPeer(UvTcpPeer const&) = delete;
	UvTcpPeer& operator=(UvTcpPeer const&) = delete;

	inline void ReadStart() noexcept {
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
				self->Close();
			}
		});
	}
	inline virtual int Send(char const* const& buf, ssize_t const& dataLen) noexcept {
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
				container_of(req->handle, UvTcpPeer, uvTcp)->Close(true);
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
	inline virtual void OnAccept(std::weak_ptr<UvTcpPeer> peer) noexcept {};
};

template<typename ListenerType>
inline std::weak_ptr<ListenerType> UvLoop::CreateListener(std::string const& ip, int const& port, int const& backlog) noexcept {
	auto listener = std::make_shared<ListenerType>();
	std::weak_ptr<ListenerType> rtv(listener);

	if (listener->Init(&uvLoop)) return rtv;

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

		auto& items = container_of(server->loop, UvLoop, uvLoop)->items;
		peer->indexAtContainer = (int)items.size();
		items.push_back(peer);

		if (uv_accept(server, (uv_stream_t*)&peer->uvTcp)) {
			peer->Close();
			return;
		}
		peer->ReadStart();
		self->OnAccept(peer);
	})) goto LabEnd;

	listener->indexAtContainer = (int)items.size();
	items.push_back(listener);
	return rtv;
LabEnd:
	uv_close((uv_handle_t*)&listener->uvTcp, nullptr);
	return rtv;
}

struct UvTcpClient : UvItem, std::enable_shared_from_this<UvTcpClient> {
	UvLoop& loop;
	int serial = 0;
	// todo: map: serial, req
	// todo: Connect timeout support

	UvTcpClient(UvLoop& loop) noexcept : loop(loop) {}
	UvTcpClient(UvTcpClient const&) = delete;
	UvTcpClient& operator=(UvTcpClient const&) = delete;

	int Connect(std::string const& ip, int const& port, int const& timeoutMS = 0) noexcept {
		struct uv_connect_t_ex : uv_connect_t {
			std::shared_ptr<UvTcpPeer> peer;
			std::weak_ptr<UvTcpClient> client;
			int serial;
		} *req = nullptr;

		auto peer = OnCreatePeer();
		if (peer->Init(&loop.uvLoop)) return -1;

		sockaddr_in6 addr;
		if (ip.find(':') == std::string::npos) {								// ipv4
			if (uv_ip4_addr(ip.c_str(), port, (sockaddr_in*)&addr)) goto LabEnd;
		}
		else {																	// ipv6
			if (uv_ip6_addr(ip.c_str(), port, &addr)) goto LabEnd;
		}

		req = new uv_connect_t_ex();
		req->peer = peer;
		req->client = shared_from_this();
		req->serial = ++serial;

		if (uv_tcp_connect(req, &peer->uvTcp, (sockaddr*)&addr, [](uv_connect_t* req_, int status) {
			auto req = (uv_connect_t_ex*)req_;
			auto peer = std::move(req->peer);
			auto client = req->client.lock();
			auto serial = req->serial;
			delete req;

			if (!client) {														// client is disposed
				uv_close((uv_handle_t*)&peer->uvTcp, nullptr);
				return;
			}
			if (status) {
				uv_close((uv_handle_t*)&peer->uvTcp, nullptr);
				client->OnConnect(serial, std::weak_ptr<UvTcpPeer>());			// connect failed.
				return;
			}

			auto& items = client->loop.items;
			peer->indexAtContainer = (int)items.size();
			items.push_back(peer);

			peer->ReadStart();
			client->OnConnect(serial, peer);									// connect success
		})) goto LabEnd;

		return serial;	// todo: insert to dict & return serial key

	LabEnd:
		uv_close((uv_handle_t*)&peer->uvTcp, nullptr);
		return -1;
	}

	virtual std::shared_ptr<UvTcpPeer> OnCreatePeer() noexcept = 0;
	inline virtual void OnConnect(int const& serial, std::weak_ptr<UvTcpPeer> peer) noexcept {};
};

template<typename ClientType>
inline std::weak_ptr<ClientType> UvLoop::CreateClient() noexcept {
	auto client = std::make_shared<ClientType>(*this);
	std::weak_ptr<ClientType> rtv(client);
	client->indexAtContainer = (int)items.size();
	items.push_back(client);
	return rtv;
}
