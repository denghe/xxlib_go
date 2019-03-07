﻿#include "xx_uv.h"

int main(int argc, char* argv[]) {
	if (argc < 2) {
		std::cout << "need args: port\n";
		return -1;
	}
	xx::UvLoop loop;
	auto listener = xx::Make<xx::UvUdpKcpListener<>>(loop, "0.0.0.0", std::atoi(argv[1]));
	listener->OnAccept = [&loop](auto& peer) {
		xx::Cout(peer->kcp->conv, " accepted.\n");
		auto timeouter = xx::TryMake<xx::UvTimer>(loop, 2000, 0);	// 两秒内没数据的连接杀掉
		auto timeouter_w = xx::Weak(timeouter);
		timeouter->OnFire = [timeouter, peer_w = xx::Weak(peer)] {
			if (auto peer = peer_w.lock()) {
				peer->Dispose();
				xx::Cout("timeout. peer dispose\n");
			}
			timeouter->OnFire = nullptr;	// kill timer
		};
		peer->OnReceivePush = [timeouter_w, peer](xx::Object_s&& msg)->int {
			if (int r = peer->SendPush(msg)) return r;	// echo
			//peer->Flush();
			return timeouter_w.lock()->Restart();
		};
		peer->OnReceiveRequest = [timeouter_w, peer](int const& serial, xx::Object_s&& msg)->int {
			if (int r = peer->SendResponse(serial, msg)) return r;	// echo
			//peer->Flush();
			return timeouter_w.lock()->Restart();
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
