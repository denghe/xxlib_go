#include "uv.h"
#include <string>
#include <memory>
#include <cassert>
#include <vector>

struct UvTcpPeer;
struct UvLoop : uv_loop_t {
	std::vector<std::shared_ptr<UvTcpPeer>> peers;
	int Init() noexcept {
		return uv_loop_init(this);
	}
};

struct UvTcpPeer : uv_tcp_t, std::enable_shared_from_this<UvTcpPeer> {
	size_t indexAtPeers = -1;
	int Init(uv_loop_t* const& loop) noexcept {
		return uv_tcp_init(loop, this);
	}
	void ReadStart() {
		uv_read_start((uv_stream_t*)this, [](uv_handle_t* h, size_t suggested_size, uv_buf_t* buf) {
			buf->base = (char*)malloc(suggested_size);
			buf->len = decltype(uv_buf_t::len)(suggested_size);
		}, [](uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
			auto self = (UvTcpPeer*)stream;
			if (nread > 0) {
				if (self->Unpack(buf->base, nread)) {
					free(buf->base);
					return;
				}
			}
			free(buf->base);
			if (nread < 0) {
				self->Close();
			}
		});
	}
	int Send(char const* const& buf, ssize_t const& dataLen) {
		struct uv_write_t_ex : uv_write_t {
			uv_buf_t buf;
		};
		auto req = (uv_write_t_ex*)malloc(sizeof(uv_write_t_ex) + dataLen);
		memcpy(req + 1, buf, dataLen);
		req->buf.base = (char*)(req + 1);
		req->buf.len = decltype(uv_buf_t::len)(dataLen);
		return uv_write(req, (uv_stream_t*)this, &req->buf, 1, [](uv_write_t *req, int status) {
			free(req);
			if (status) return;
		});
	}
	virtual void Close() {
		auto h = (uv_handle_t*)this;
		assert(!uv_is_closing(h));
		uv_close(h, [](uv_handle_t* h) {
			auto self = (UvTcpPeer*)h;
			auto& peers = ((UvLoop*)self->loop)->peers;
			auto idx = self->indexAtPeers;
			assert(idx != -1 && idx < peers.size());
			auto last = peers.size() - 1;
			if (idx < last) {
				peers[last]->indexAtPeers = idx;
				peers[idx] = std::move(peers[last]);
			}
			peers.pop_back();
		});
	}
	virtual int Unpack(char const* const& buf, size_t const& len) = 0;
};

template<typename PeerType = UvTcpPeer>
struct UvTcpListener : uv_tcp_t {
	sockaddr_in6 addr;
	int Init(UvLoop& uvloop) noexcept {
		return uv_tcp_init(&uvloop, this);
	}
	int SetAddress(std::string const& ipv4, int const& port) noexcept {
		return uv_ip4_addr(ipv4.c_str(), port, (sockaddr_in*)&addr);
	}
	int SetAddress6(std::string const& ipv6, int const& port) noexcept {
		return uv_ip6_addr(ipv6.c_str(), port, &addr);
	}
	int Bind() noexcept {
		return uv_tcp_bind(this, (sockaddr*)&addr, 0);
	}
	int Listen(int const& backlog = 128) {
		return uv_listen((uv_stream_t*)this, backlog, [](uv_stream_t* server, int status) {
			if (status) return;

			auto peer = std::make_shared<PeerType>();
			if (peer->Init(server->loop)) return;
			auto& peers = ((UvLoop*)server->loop)->peers;
			peer->indexAtPeers = peers.size();
			peers.push_back(peer);

			if (uv_accept(server, (uv_stream_t*)&*peer)) {
				peer->Close();
				return;
			}
			auto self = (UvTcpListener*)server;
			self->ReadStart();
			self->OnAccept(peer);
		});
	}
	virtual void OnAccept(std::weak_ptr<PeerType> peer) = 0;
};

struct Peer : UvTcpPeer {

};
//struct Listener : UvTcpListener {
//
//};


int main() {
	return 0;
}
