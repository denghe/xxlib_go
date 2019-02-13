#pragma once
#include "uv.h"
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

struct UvItems {
	int indexAtContainer = -1;
};

struct UvLoop {
	uv_loop_t uvLoop;
	std::vector<std::shared_ptr<UvItems>> items;

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

	~UvLoop() {
		items.clear();
		int r = uv_loop_close(&uvLoop);
		assert(!r);
	}
};

struct UvTcp : UvItems {
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
				nread = self->Unpack(buf->base, nread);
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
	inline virtual int Unpack(char const* const& buf, size_t const& len) noexcept = 0;
};

struct UvTcpListener : UvTcp {
	sockaddr_in6 addr;

	UvTcpListener() = default;
	UvTcpListener(UvTcpListener const&) = delete;
	UvTcpListener& operator=(UvTcpListener const&) = delete;

	inline int SetAddress(std::string const& ipv4, int const& port) noexcept {
		return uv_ip4_addr(ipv4.c_str(), port, (sockaddr_in*)&addr);
	}
	inline int SetAddress6(std::string const& ipv6, int const& port) noexcept {
		return uv_ip6_addr(ipv6.c_str(), port, &addr);
	}
	inline int Bind() noexcept {
		return uv_tcp_bind(&uvTcp, (sockaddr*)&addr, 0);
	}
	inline int Listen(int const& backlog = 128) noexcept {
		return uv_listen((uv_stream_t*)&uvTcp, backlog, [](uv_stream_t* server, int status) {
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
		});
	}
	inline virtual std::shared_ptr<UvTcpPeer> OnCreatePeer() noexcept = 0;
	inline virtual void OnAccept(std::weak_ptr<UvTcpPeer> peer) noexcept {};
};

template<typename ListenerType>
inline std::weak_ptr<ListenerType> UvLoop::CreateListener(std::string const& ip, int const& port, int const& backlog) noexcept {
	auto listener = std::make_shared<ListenerType>();
	std::weak_ptr<ListenerType> rtv(listener);

	if (listener->Init(&uvLoop)) return rtv;
	if (ip.find(':') == std::string::npos) {
		if (listener->SetAddress(ip, port)) goto LabEnd;
	}
	else if (listener->SetAddress6(ip, port)) goto LabEnd;
	if (listener->Bind()) goto LabEnd;
	if (listener->Listen(backlog)) goto LabEnd;
	listener->indexAtContainer = (int)items.size();
	items.push_back(listener);
	return rtv;
LabEnd:
	uv_close((uv_handle_t*)&listener->uvTcp, nullptr);
	return rtv;
}
