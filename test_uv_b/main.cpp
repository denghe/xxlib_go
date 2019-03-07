#include "xx_uv.h"

struct Peer : xx::UvUdpKcpPeer {
	using xx::UvUdpKcpPeer::UvUdpKcpPeer;
	std::chrono::time_point<std::chrono::system_clock> last;
	inline int SendData() {
		last = std::chrono::system_clock::now();
		auto msg = xx::TryMake<xx::BBuffer>();
		assert(msg);
		return SendRequest(msg, [this](xx::Object_s&& msg) {
			if (!msg) {
				std::cout << "timeout. retry";
			}
			else {
				auto elapsedSec = double(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now() - last).count()) / 1000000000.0;
				std::cout << elapsedSec << std::endl;
			}
			return SendData();
		}, 2000);
	}
};
int main(int argc, char* argv[]) {
	if (argc < 3) {
		std::cout << "need args: ip port\n";
		return -1;
	}
	xx::UvLoop loop;
	auto dialer = xx::Make<xx::UvUdpKcpDialer<Peer>>(loop);
	dialer->OnConnect = [&dialer] {
		dialer->peer->SendData();
	};
	dialer->Dial(argv[1], std::atoi(argv[2]));
	loop.Run();
	std::cout << "end.";
	return 0;
}

//int main(int argc, char* argv[]) {
//	if (argc < 3) {
//		std::cout << "need args: ip port\n";
//		return -1;
//	}
//	xx::UvLoop loop;
//	int count = 0;
//	std::chrono::time_point<std::chrono::system_clock> lastTime = std::chrono::system_clock::now();
//	auto dialer = xx::Make<xx::UvUdpKcpDialer<>>(loop);
//	dialer->OnConnect = [&] {
//		dialer->peer->OnDisconnect = [] {
//			xx::Cout("disconnected.\n");
//		};
//		dialer->peer->OnReceivePush = [&](xx::Object_s&& msg)->int {
//			if (++count > 100) {
//				xx::Cout(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - lastTime).count(), "\n");
//				return -1;
//			}
//			int r = dialer->peer->SendPush(msg);		// echo
//			//dialer->peer->Flush();
//			return r;
//		};
//		auto msg = xx::Make<xx::BBuffer>();
//		msg->Write(1, 2, 3, 4, 5);
//		for (int i = 0; i < 1; ++i) {
//			dialer->peer->SendPush(msg);
//		}
//	};
//	dialer->Dial(argv[1], std::atoi(argv[2]));
//	loop.Run();
//	return 0;
//}
