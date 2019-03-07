#include "xx_uv.h"

int count = 0;
std::chrono::time_point<std::chrono::system_clock> lastTime = std::chrono::system_clock::now();
int main(int argc, char* argv[]) {

	xx::UvLoop loop;
	auto dialer = xx::Make<xx::UvUdpKcpDialer<>>(loop);
	dialer->OnConnect = [&] {
		dialer->peer->OnDisconnect = [] {
			xx::Cout("disconnected.\n");
		};
		dialer->peer->OnReceivePush = [&](xx::Object_s&& msg)->int {
			if (++count > 100) {
				xx::Cout(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - lastTime).count(), "\n");
				return -1;
			}
			int r = dialer->peer->SendPush(msg);		// echo
			//dialer->peer->Flush();
			return r;
		};
		auto msg = xx::Make<xx::BBuffer>();
		msg->Write(1, 2, 3, 4, 5);
		for (int i = 0; i < 1; ++i) {
			dialer->peer->SendPush(msg);
		}
	};
	dialer->Dial("127.0.0.1", 12345);
	loop.Run();
	return 0;
}
