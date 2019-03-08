#include "xx_uv.h"

// todo: 已知问题, server 重启后似乎无法恢复

auto msg = xx::Make<xx::BBuffer>();
struct Peer : xx::UvUdpKcpPeer {
	std::chrono::time_point<std::chrono::system_clock> last;

	using xx::UvUdpKcpPeer::UvUdpKcpPeer;
	~Peer() { if (!this->Disposed()) this->Dispose(); }

	inline void SendData() {
		last = std::chrono::system_clock::now();
		SendRequest(msg, [this](xx::Object_s&& msg) {
			if (!msg) {
				std::cout << "timeout. retry";
			}
			else {
				auto elapsedSec = double(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now() - last).count()) / 1000000000.0;
				std::cout << elapsedSec << std::endl;
			}
			SendData();
			return 0;
		}, 2000);
	}
};

int main(int argc, char* argv[]) {
	if (argc < 3) {
		std::cout << "need args: ip port\n";
		return -1;
	}
	xx::Uv loop;
	auto dialer = xx::Make<xx::UvUdpKcpDialer<Peer>>(loop);
	dialer->OnConnect = [&dialer] (std::shared_ptr<Peer>& peer){
		if (peer) {
			peer->SendData();
		}
	};
	dialer->Dial(argv[1], std::atoi(argv[2]));
	loop.Run();
	std::cout << "end.";
	return 0;
}
