#include "xx_uv.h"

struct EchoPeer : xx::UvUdpKcpPeer {
	xx::UvTimer_s timeouter;	// 两秒内没数据的连接杀掉
	EchoPeer(xx::UvUdpBasePeer* const& owner, xx::Guid const& g)
		: xx::UvUdpKcpPeer(owner, g) {
		timeouter = xx::Make<xx::UvTimer>(loop, 2000, 0, [this] {
			Dispose();
			xx::Cout("timeout. peer dispose\n");
		});	
		OnReceivePush = [this](xx::Object_s&& msg)->int {
			if (int r = this->SendPush(msg)) return r;
			//peer->Flush();
			return timeouter->Restart();
		};
		OnReceiveRequest = [this](int const& serial, xx::Object_s&& msg)->int {
			if (int r = this->SendResponse(serial, msg)) return r;
			//peer->Flush();
			return timeouter->Restart();
		};
	}
};

int main(int argc, char* argv[]) {
	if (argc < 2) {
		std::cout << "need args: port\n";
		return -1;
	}
	xx::UvLoop loop;
	auto listener = xx::Make<xx::UvUdpKcpListener<EchoPeer>>(loop, "0.0.0.0", std::atoi(argv[1]));
	listener->OnAccept = [&loop](std::shared_ptr<EchoPeer>& peer) {
		xx::Cout(peer->kcp->conv, " accepted.\n");
		peer->OnDisconnect = [peer] {};	// hold memory
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
