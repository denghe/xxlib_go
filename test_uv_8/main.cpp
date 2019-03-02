#include "xx_uv.h"
#include <unordered_set>

struct EchoPeer : xx::UvTcpBasePeer {
	inline int Unpack(uint8_t* const& buf, uint32_t const& len) noexcept override {
		return Send(buf, len);
	}
};
int main(int argc, char* argv[]) {
	if (argc < 2) {
		std::cout << "need args: port\n";
		return -1;
	}
	xx::UvLoop loop;
	auto listener = loop.CreateTcpListener<xx::UvTcpListener<EchoPeer>>("0.0.0.0", std::atoi(argv[1]));
	listener->OnAccept = [](std::shared_ptr<EchoPeer>&& peer) {
		peer->OnDisconnect = [peer] {};								// hold memory
	};
	loop.Run();
	return 0;
}
