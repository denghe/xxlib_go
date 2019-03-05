#include "xx_uv.h"
namespace xx {
	struct uv_udp_send_t_ex : uv_udp_send_t {
		uv_buf_t buf;
	};

	struct UvUdpBase : UvItem {
		uv_udp_t* uvUdp = nullptr;
		sockaddr_in6 addr;
		std::array<char, 65536> recvBuf;
		std::function<void()> OnDispose;

		UvUdpBase(UvLoop& loop, std::string const& ip, int const& port, bool isListener) 
			: UvItem(loop) {
			if (ip.size()) {
				if (ip.find(':') == std::string::npos) {
					if (int r = uv_ip4_addr(ip.c_str(), port, (sockaddr_in*)&addr)) throw r;
				}
				else {
					if (int r = uv_ip6_addr(ip.c_str(), port, &addr)) throw r;
				}
			}
			uvUdp = UvAlloc<uv_udp_t>(this);
			if (!uvUdp) throw -2;
			if (int r = uv_udp_init(&loop.uvLoop, uvUdp)) {
				uvUdp = nullptr;
				throw r;
			}
			xx::ScopeGuard sgUdp([this] { UvHandleCloseAndFree(uvUdp); });
			if (isListener) {
				if (int r = uv_udp_bind(uvUdp, (sockaddr*)&addr, UV_UDP_REUSEADDR)) throw r;
			}
			if (int r = uv_udp_recv_start(uvUdp, UvAllocCB, [](uv_udp_t* handle, ssize_t nread, const uv_buf_t* buf, const struct sockaddr* addr, unsigned flags) {
				if (nread > 0) {
					nread = UvGetSelf<UvUdpBase>(handle)->Unpack((uint8_t*)buf->base, (uint32_t)nread, addr);
				}
				if (buf) ::free(buf->base);
				if (nread < 0) {
					UvGetSelf<UvUdpBase>(handle)->Dispose(true);
				}
			})) throw r;
			sgUdp.Cancel();
		}

		virtual ~UvUdpBase() {
			UvHandleCloseAndFree(uvUdp);
		}

		inline bool Disposed() noexcept {
			return uvUdp == nullptr;
		}

		inline virtual void Dispose(bool callback = false) {
			if (!uvUdp) return;
			UvHandleCloseAndFree(uvUdp);
			if (callback && OnDispose) {
				OnDispose();
				OnDispose = nullptr;
			}
		}

		virtual int Unpack(uint8_t* const& recvBuf, uint32_t const& recvLen, sockaddr const* const& addr) noexcept {
			// todo: print addr
			for (uint32_t i = 0; i < recvLen; i++) {
				std::cout << (int)recvBuf[i] << " ";
			}
			return 0;
		}

		// reqbuf = uv_udp_send_t_ex space + len space + data
		// len = data's len
		inline int SendReqPack(uint8_t* const& reqbuf, uint32_t const& len, sockaddr const* const& addr = nullptr) {
			reqbuf[sizeof(uv_udp_send_t_ex) + 0] = uint8_t(len);		// fill package len
			reqbuf[sizeof(uv_udp_send_t_ex) + 1] = uint8_t(len >> 8);
			reqbuf[sizeof(uv_udp_send_t_ex) + 2] = uint8_t(len >> 16);
			reqbuf[sizeof(uv_udp_send_t_ex) + 3] = uint8_t(len >> 24);

			auto req = (uv_udp_send_t_ex*)reqbuf;						// fill req args
			req->buf.base = (char*)(req + 1);
			req->buf.len = decltype(uv_buf_t::len)(len + 4);
			return Send(req, addr);
		}

		inline int Send(uv_udp_send_t_ex* const& req, sockaddr const* const& addr = nullptr) noexcept {
			if (!uvUdp) return -1;
			// todo: check send queue len ? protect?  uv_stream_get_write_queue_size((uv_stream_t*)uvTcp);
			int r = uv_udp_send(req, uvUdp, &req->buf, 1, addr ? addr : (sockaddr*)&this->addr, [](uv_udp_send_t *req, int status) {
				::free(req);
			});
			if (r) Dispose();
			return r;
		}

		inline int Send(uint8_t const* const& buf, ssize_t const& dataLen, sockaddr const* const& addr = nullptr) noexcept {
			if (!uvUdp) return -1;
			auto req = (uv_udp_send_t_ex*)::malloc(sizeof(uv_udp_send_t_ex) + dataLen);
			memcpy(req + 1, buf, dataLen);
			req->buf.base = (char*)(req + 1);
			req->buf.len = decltype(uv_buf_t::len)(dataLen);
			return Send(req, addr);
		}
		inline int Send(char const* const& buf, ssize_t const& dataLen, sockaddr const* const& addr = nullptr) noexcept {
			return Send((uint8_t*)buf, dataLen, addr);
		}
	};

	std::shared_ptr<UvUdpBase> CreateUvUdpBase(xx::UvLoop& loop, std::string const& ip, int const& port, bool isListener) {
		return xx::TryMake<UvUdpBase>(loop, ip, port, isListener);
	}
}

int main(int argc, char* argv[]) {
	xx::UvLoop loop;
	auto receiver = xx::TryMake<xx::UvUdpBase>(loop, "0.0.0.0", 12345, true);
	assert(receiver);

	auto timer = xx::TryMake<xx::UvTimer>(loop, 100, 0);
	assert(timer);
	timer->OnFire = [&loop, timer] {
		auto sender = xx::TryMake<xx::UvUdpBase>(loop, "127.0.0.1", 12345, false);
		assert(sender);
		sender->Send("1234", 4);
		timer->OnFire = nullptr;
	};

	loop.Run();
	return 0;
}
