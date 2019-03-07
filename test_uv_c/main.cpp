#include "xx_uv.h"
struct KcpPeer : xx::UvUdpBasePeer {
	using ThisType = KcpPeer;
	using BaseType = xx::UvUdpBasePeer;
	xx::Guid g;
	ikcpcb *kcp = nullptr;
	std::chrono::time_point<std::chrono::steady_clock> lastTime;
	xx::UvTimer_s updater;
	KcpPeer(xx::UvLoop& loop, std::string const& ip, int const& port, bool isListener, xx::Guid const& g)
		: BaseType(loop, ip, port, isListener)
		, g(g) {
		kcp = ikcp_create(g, this);
		if (!kcp) throw - 1;
		xx::ScopeGuard sgKcp([&] { ikcp_release(kcp); kcp = nullptr; });
		ikcp_nodelay(kcp, 1, 10, 2, 1);
		ikcp_wndsize(kcp, 128, 128);
		kcp->rx_minrto = 10;
		ikcp_setoutput(kcp, [](const char *buf, int len, ikcpcb *kcp, void *user)->int {
			return ((ThisType*)user)->BaseType::Send(buf, len, (sockaddr*)&((ThisType*)user)->addr);
		});
		lastTime = std::chrono::steady_clock::now();
		updater = xx::TryMake<xx::UvTimer>(loop, 0, 10);
		if (!updater) throw - 2;
		updater->OnFire = [this] {
			auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - lastTime).count();
			Update((int)ms);
		};
		sgKcp.Cancel();
	}
	~KcpPeer() {
		Dispose(false);
	}
	inline virtual void Dispose(bool callback = false) noexcept override {
		if (kcp) {
			ikcp_release(kcp);
			kcp = nullptr;
			updater.reset();
		}
		this->BaseType::Dispose(callback);
	}
	virtual int Unpack(uint8_t* const& recvBuf, uint32_t const& recvLen, sockaddr const* const& addr) noexcept override {
		assert(kcp);
		assert(uvUdp);
		memcpy(&this->addr, addr, sizeof(this->addr));
		int r = ikcp_input(kcp, (char*)recvBuf, recvLen);
		assert(!r);
		return r;
	}
	void Update(int ms) {
		assert(kcp);
		assert(uvUdp);
		ikcp_update(kcp, ms);
		do {
			char buf[2048];
			int recvLen = ikcp_recv(kcp, buf, 2048);
			if (recvLen <= 0) break;
			if (Unpack(buf, recvLen)) {
				Dispose();
				return;
			}
		} while (true);
	}
	virtual int Unpack(char* const& recvBuf, uint32_t const& recvLen) noexcept = 0;

	virtual int Send(char* const& buf, int const& len) {
		assert(kcp);
		int r = ikcp_send(kcp, buf, len);
		assert(!r);
		return r;
	}
};

struct KcpEchoPeer : KcpPeer {
	using KcpPeer::KcpPeer;
	virtual int Unpack(char* const& recvBuf, uint32_t const& recvLen) noexcept override {
		int r = Send(recvBuf, recvLen);		// echo
		assert(!r);
		return r;
	}
};

void RunServer() {
	xx::UvLoop loop;
	auto listener = xx::Make<KcpEchoPeer>(loop, "0.0.0.0", 12345, true, xx::Guid(false));
	loop.Run();
}

struct KcpClientPeer : KcpPeer {
	using KcpPeer::KcpPeer;
	size_t count = 0;
	std::chrono::time_point<std::chrono::system_clock> t = std::chrono::system_clock::now();
	virtual int Unpack(char* const& recvBuf, uint32_t const& recvLen) noexcept override {
		if (++count >= 10000) {
			std::cout << double(std::chrono::nanoseconds(std::chrono::system_clock::now() - t).count()) / 1000000000 << std::endl;
			return -1;
		}
		return 0;
	}
};

void RunClient() {
	xx::UvLoop loop;
	auto dialer = xx::Make<KcpClientPeer>(loop, "127.0.0.1", 12345, false, xx::Guid(false));
	auto timer = xx::Make<xx::UvTimer>(loop, 0, 10);
	timer->OnFire = [&] {
		if (!dialer->Disposed()) {
			for (size_t i = 0; i < 100; i++) {
				dialer->Send("a", 1);
			}
		}
		else {
			dialer.reset();
			timer.reset();
		}
	};
	loop.Run();
}

int main(int argc, char* argv[]) {
	std::thread t1{ RunServer };
	std::thread t2{ RunClient };
	t1.join();
	t2.join();
	return 0;
}






//#include "xx_uv.h"
//struct UvUdpEchoPeer : xx::UvUdpBasePeer {
//	using xx::UvUdpBasePeer::UvUdpBasePeer;
//	virtual int Unpack(uint8_t* const& recvBuf, uint32_t const& recvLen, sockaddr const* const& addr) noexcept override {
//		return Send(recvBuf, recvLen, addr);
//	}
//};
//int main(int argc, char* argv[]) {
//	xx::UvLoop loop;
//	auto listener = xx::TryMake<UvUdpEchoPeer>(loop, "0.0.0.0", 12345, true);
//	assert(listener);
//	loop.Run();
//	return 0;
//}
//
//
//#include "xx_uv.h"
//struct UvUdpClientPeer : xx::UvUdpBasePeer {
//	using xx::UvUdpBasePeer::UvUdpBasePeer;
//	size_t count = 0;
//	std::chrono::time_point<std::chrono::system_clock> t = std::chrono::system_clock::now();
//	virtual int Unpack(uint8_t* const& recvBuf, uint32_t const& recvLen, sockaddr const* const& addr) noexcept override {
//		if (++count >= 100000) {
//			std::cout << double(std::chrono::nanoseconds(std::chrono::system_clock::now() - t).count()) / 1000000000 << std::endl;
//			return -1;
//		}
//		return Send(recvBuf, recvLen, addr);
//	}
//};
//
//int main2(int argc, char* argv[]) {
//	xx::UvLoop loop;
//	auto dialer = xx::TryMake<UvUdpClientPeer>(loop, "127.0.0.1", 12345, false);
//	assert(dialer);
//	dialer->Send("a", 1);
//	loop.Run();
//	return 0;
//}
