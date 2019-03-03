#include "xx_uv.h"

struct EchoPeer : xx::UvTcpBasePeer {
	xx::UvTimer_s timeouter;
	inline int Unpack(uint8_t* const& buf, uint32_t const& len) noexcept override {
		std::cout << timeouter->Restart();	// 重置超时时间
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
	listener->OnAccept = [&loop](std::shared_ptr<EchoPeer>&& peer) {
		peer->OnDisconnect = [peer] {}; // hold memory
		peer->timeouter = loop.CreateTimer(5000, 0, [peer_w = xx::Weak(peer)]{	// 5 秒超时
			peer_w.lock()->Dispose();
		});
	};
	loop.Run();
	return 0;
}
