#include "xx_uv.h"
int count = 0;
std::chrono::time_point<std::chrono::system_clock> lastTime = std::chrono::system_clock::now();
int main(int argc, char* argv[]) {
	xx::UvLoop loop;
	auto dialer = xx::TryMake<xx::UvUdpKcpDialer<>>(loop);
	assert(dialer);
	auto msg = xx::TryMake<xx::BBuffer>();
	assert(msg);
	msg->Write(1, 2, 3, 4, 5);
	dialer->OnConnect = [&] {
		dialer->peer->OnDisconnect = [] {
			xx::Cout("disconnected.");
		};
		auto timer = xx::TryMake<xx::UvTimer>(loop, 0, 50);
		timer->OnFire = [&, timer] {
			if (dialer->peer && !dialer->peer->Disposed()) {
				int r = dialer->peer->SendPush(msg);
				assert(!r);
				dialer->peer->Flush();
				xx::Cout(".");
			}
			else xx::Cout("!");

		};
		//dialer->peer->OnReceivePush = [&, timer](xx::Object_s&& msg)->int {
		//	if (++count > 100) {
		//		std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - lastTime).count();
		//		return -1;
		//	}
		//	return 0;
		//};
	};
	dialer->Dial("127.0.0.1", 12345);
	loop.Run();
	return 0;
}
