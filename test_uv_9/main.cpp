#include "xx_uv.h"
struct Peer : xx::UvTcpPeer {
	int64_t last;
	inline int SendData() {
		last = std::chrono::system_clock::now().time_since_epoch().count();
		return SendRequest(xx::BBuffer::MakeShared(), [this](xx::Object_s&& msg) {
			if (!msg) {
				std::cout << "timeout. retry";
			}
			else {
				auto elapsedSec = double(std::chrono::system_clock::now().time_since_epoch().count() - last) / 10000000.0;
				std::cout << elapsedSec << std::endl;
			}
			return SendData();
		}, 2000);
	}
};
struct Dialer : xx::UvTcpDialer {
	using xx::UvTcpDialer::UvTcpDialer;
	inline virtual xx::UvTcpBasePeer_s CreatePeer() noexcept {
		return std::make_shared<Peer>();
	}
	inline virtual void Connect() noexcept override {
		PeerAs<Peer>()->SendData();
	}
};
int main(int argc, char* argv[]) {
	if (argc < 3) {
		std::cout << "need args: ip port\n";
		return -1;
	}
	xx::UvLoop loop;
	auto dialer = loop.CreateTcpDialer<Dialer>();
	dialer->Dial(argv[1], std::atoi(argv[2]));
	loop.Run();
	std::cout << "end.";
	return 0;
}
