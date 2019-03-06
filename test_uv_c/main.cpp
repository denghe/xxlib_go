#include "xx_uv.h"
struct UvUdpEchoPeer : xx::UvUdpBasePeer {
	using xx::UvUdpBasePeer::UvUdpBasePeer;
	virtual int Unpack(uint8_t* const& recvBuf, uint32_t const& recvLen, sockaddr const* const& addr) noexcept override {
		return Send(recvBuf, recvLen, addr);
	}
};
int main(int argc, char* argv[]) {
	xx::UvLoop loop;
	auto listener = xx::TryMake<UvUdpEchoPeer>(loop, "0.0.0.0", 12345, true);
	assert(listener);
	loop.Run();
	return 0;
}


#include "xx_uv.h"
struct UvUdpClientPeer : xx::UvUdpBasePeer {
	using xx::UvUdpBasePeer::UvUdpBasePeer;
	size_t count = 0;
	std::chrono::time_point<std::chrono::system_clock> t = std::chrono::system_clock::now();
	virtual int Unpack(uint8_t* const& recvBuf, uint32_t const& recvLen, sockaddr const* const& addr) noexcept override {
		if (++count >= 100000) {
			std::cout << double(std::chrono::nanoseconds(std::chrono::system_clock::now() - t).count()) / 1000000000 << std::endl;
			return -1;
		}
		return Send(recvBuf, recvLen, addr);
	}
};

int main2(int argc, char* argv[]) {
	xx::UvLoop loop;
	auto dialer = xx::TryMake<UvUdpClientPeer>(loop, "127.0.0.1", 12345, false);
	assert(dialer);
	dialer->Send("a", 1);
	loop.Run();
	return 0;
}
