#include "xx_uv.h"

int main(int argc, char* argv[]) {
	xx::UvLoop loop;
	auto listener = xx::TryMake<xx::UvUdpKcpListener<>>(loop, "0.0.0.0", 12345);
	assert(listener);
	listener->OnAccept = [&loop](xx::UvUdpKcpPeer_s& peer) {
		xx::Cout(peer->kcp->conv, "\n");
		auto timeouter = xx::TryMake<xx::UvTimer>(loop, 2000, 0);
		timeouter->OnFire = [timeouter, peer] {	// mem holder
			if (!peer->Disposed()) {
				peer->Dispose();
				xx::Cout("timeout. peer dispose\n");
			}
			timeouter->OnFire = nullptr;	// kill timer
		};
		peer->OnReceivePush = [timeouter, peer](xx::Object_s&& msg)->int {		// hold memory
			xx::Cout("recv ", msg, "\n");
			peer->SendPush(msg);	// echo
			peer->Flush();
			timeouter->Restart();
			return 0;
		};
	};
	loop.Run();
	return 0;
}
