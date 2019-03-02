#include "xx_uv.h"
#include <unordered_set>
int main(int argc, char* argv[]) {
	if (argc < 2) {
		std::cout << "need args: port\n";
		return -1;
	}
	xx::UvLoop loop;
	auto listener = loop.CreateTcpListener<xx::UvTcpListener>("0.0.0.0", std::atoi(argv[1]));
	assert(listener);
	listener->OnAccept = [](xx::UvTcpPeer_s&& peer) {
		peer->OnReceiveRequest = [peer](int const& serial, xx::Object_s&& msg)->int {
			return peer->SendResponse(serial, msg);
		};
		peer->OnDisconnect = [peer]{
			peer->OnReceiveRequest = nullptr;
			peer->OnDisconnect = nullptr;
		};
	};
	loop.Run();
	std::cout << "end.\n";
	return 0;
}
