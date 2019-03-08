#include "xx_uv.h"

struct EchoPeer : xx::UvUdpKcpPeer {
	using BaseType = xx::UvUdpKcpPeer;
	xx::UvTimer_s timeouter;				// 两秒内没数据的连接杀掉
	std::shared_ptr<EchoPeer> holder;

	EchoPeer(xx::UvUdpBasePeer* const& owner, xx::Guid const& g)
		: BaseType(owner, g) {
		timeouter = xx::Make<xx::UvTimer>(loop);
		if (int r = timeouter->Start(2000, 0, [this] {
			Dispose();	// unhold memory
			xx::Cout("timeout. peer dispose\n");
		})) throw r;
	}

	virtual void OnDisconnect() noexcept override {
		holder = nullptr;		// unhold memory
	}
	virtual int OnReceivePush(xx::Object_s&& msg) noexcept override {
		if (int r = this->SendPush(msg)) return r;
		//peer->Flush();
		return timeouter->Restart();
	}
	virtual int OnReceiveRequest(int const& serial, xx::Object_s&& msg) noexcept override {
		if (int r = this->SendResponse(serial, msg)) return r;
		//peer->Flush();
		return timeouter->Restart();
	}
};

int main(int argc, char* argv[]) {
	if (argc < 2) {
		std::cout << "need args: port\n";
		return -1;
	}
	xx::Uv uv;
	auto listener = xx::Make<xx::UvUdpKcpListener<EchoPeer>>(uv, "0.0.0.0", std::atoi(argv[1]));
	listener->OnAccept = [&uv](std::shared_ptr<EchoPeer>& peer) {
		peer->holder = peer;	// hold memory
		xx::Cout(peer->kcp->conv, " accepted.\n");
	};
	uv.Run();
	return 0;
}
