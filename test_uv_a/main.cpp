#include "xx_uv.h"

int main(int argc, char* argv[]) {
	xx::UvLoop loop;
	auto listener = xx::TryMake<xx::UvUdpKcpListener<>>(loop, "0.0.0.0", 12345);
	assert(listener);
	listener->OnAccept = [&loop](auto& peer) {
		xx::Cout(peer->kcp->conv, " accepted.\n");
		auto timeouter = xx::TryMake<xx::UvTimer>(loop, 2000, 0);	// 两秒内没数据的连接杀掉
		timeouter->OnFire = [timeouter, peer] {	// mem holder
			if (!peer->Disposed()) {
				peer->Dispose();
				xx::Cout("timeout. peer dispose\n");
			}
			timeouter->OnFire = nullptr;	// kill timer
		};
		peer->OnReceivePush = [timeouter, peer](xx::Object_s&& msg)->int {		// hold memory
			if (int r = peer->SendPush(msg)) return r;	// echo
			//peer->Flush();
			return timeouter->Restart();
		};
	};
	loop.Run();
	return 0;
}


//int main(int argc, char* argv[]) {
//	xx::UvLoop loop;
//	auto listener = xx::Make<xx::UvUdpKcpListener<>>(loop, "0.0.0.0", 12345);
//	listener->OnAccept = [&loop](auto& peer) {
//		xx::Cout(peer->kcp->conv, " accepted.\n");
//		peer->OnReceivePush = [peer](xx::Object_s&& msg)->int {		// hold memory
//			int r = peer->SendPush(msg);		// echo
//			peer->Flush();
//			return r;
//		};
//	};
//	loop.Run();
//	return 0;
//}
