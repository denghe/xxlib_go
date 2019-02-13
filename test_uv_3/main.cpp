#include "xx_uv_cpp.h"
#include "xx_uv_cpp_echo.h"
#include "xx_uv_cpp_package.h"
#include <chrono>
#include <iostream>

struct EchoClientPeer : UvTcpPeer {
	std::chrono::time_point<std::chrono::system_clock> t = std::chrono::system_clock::now();
	int count = 0;
	inline int Unpack(char const* const& buf, size_t const& len) noexcept override {
		Send(buf, len);
		if (++count > 100000)
		{
			std::cout << double(std::chrono::nanoseconds(std::chrono::system_clock::now() - t).count()) / 1000000000 << std::endl;
			return -1;
		}
		return 0;
	}
};

struct EchoClient : UvTcpClient {
	using UvTcpClient::UvTcpClient;
	inline virtual std::shared_ptr<UvTcpPeer> OnCreatePeer() noexcept override {
		return std::make_shared<EchoClientPeer>();
	}
	inline virtual void OnConnect(int const& serial, std::weak_ptr<UvTcpPeer> peer_) noexcept override {
		if (auto peer = peer_.lock()) {
			peer->Send("a", 1);
		}
	}
};

int main() {
	UvLoop uvloop;
	uvloop.CreateListener<EchoListener>("0.0.0.0", 12345);
	auto client = uvloop.CreateClient<EchoClient>();
	client.lock()->Connect("127.0.0.1", 12345);
	uvloop.Run();
	return 0;
}
