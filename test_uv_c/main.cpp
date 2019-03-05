#include "xx_uv.h"
namespace xx {

	struct UvUdpPeer : UvUdpBasePeer {
		BBuffer recvBB;			// for replace buf memory decode package
		BBuffer sendBB;
		std::function<int(Object_s&& msg)> OnReceivePush;
		std::function<int(int const& serial, Object_s&& msg)> OnReceiveRequest;
		std::unordered_map<int, std::pair<std::function<int(Object_s&& msg)>, UvTimer_s>> callbacks;
		int serial = 0;

		using UvUdpBasePeer::UvUdpBasePeer;
		UvUdpPeer(UvUdpPeer const&) = delete;
		UvUdpPeer& operator=(UvUdpPeer const&) = delete;

		inline virtual void Dispose(bool callback = true) noexcept override {
			if (!uvUdp) return;
			for (decltype(auto) kv : callbacks) {
				kv.second.first(nullptr);
			}
			callbacks.clear();
			auto self = shared_from_this();		// hold memory
			OnReceivePush = nullptr;
			OnReceiveRequest = nullptr;
			this->UvUdpBasePeer::Dispose(callback);
		}

		virtual int HandlePack(uint8_t* const& recvBuf, uint32_t const& recvLen, sockaddr const* const& addr) noexcept override {
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
				v.second = xx::TryMake<UvTimer>(loop, timeoutMS, 0, [this, serial = this->serial]() {
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

		// todo
	};
}

int main(int argc, char* argv[]) {
	xx::UvLoop loop;
	auto receiver = xx::TryMake<xx::UvUdpBasePeer>(loop, "0.0.0.0", 12345, true);
	assert(receiver);

	auto timer = xx::TryMake<xx::UvTimer>(loop, 100, 0);
	assert(timer);
	timer->OnFire = [&loop, timer] {
		auto sender = xx::TryMake<xx::UvUdpBasePeer>(loop, "127.0.0.1", 12345, false);
		assert(sender);
		sender->Send("1234", 4);
		timer->OnFire = nullptr;
	};

	loop.Run();
	return 0;
}
