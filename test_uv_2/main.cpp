#include "uv.h"
#include <cassert>
int Send(uv_stream_t* const& stream, char const* const& buf, ssize_t const& dataLen) {
	struct uv_write_t_ex : uv_write_t {
		uv_buf_t buf;
	};
	auto req = (uv_write_t_ex*)malloc(sizeof(uv_write_t_ex) + dataLen);
	memcpy(req + 1, buf, dataLen);
	req->buf.base = (char*)(req + 1);
	req->buf.len = decltype(uv_buf_t::len)(dataLen);
	return uv_write(req, stream, &req->buf, 1, [](uv_write_t *req, int status) {
		free(req);
		if (status) return;
	});
}
int main() {
	uv_loop_t uv;
	uv_loop_init(&uv);
	uv_tcp_t listener;
	uv_tcp_init(&uv, &listener);
	sockaddr_in addr;
	uv_ip4_addr("0.0.0.0", 12345, &addr);
	uv_tcp_bind(&listener, (sockaddr*)&addr, 0);
	uv_listen((uv_stream_t*)&listener, 128, [](uv_stream_t* server, int status) {
		if (status) return;
		auto peer = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
		uv_tcp_init(server->loop, peer);
		uv_accept(server, (uv_stream_t*)peer);
		uv_read_start((uv_stream_t*)peer, [](uv_handle_t* h, size_t suggested_size, uv_buf_t* buf) {
			buf->base = (char*)malloc(suggested_size);
			buf->len = decltype(uv_buf_t::len)(suggested_size);
		}, [](uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
			if (nread > 0) {
				Send(stream, buf->base, nread);
			}
			free(buf->base);
			if (nread < 0) {
				auto h = (uv_handle_t*)stream;
				assert(!uv_is_closing(h));
				uv_close(h, [](uv_handle_t* h) {
					free(h);
				});
			}
		});
	});
	uv_run(&uv, UV_RUN_DEFAULT);
	uv_loop_close(&uv);
	return 0;
}
