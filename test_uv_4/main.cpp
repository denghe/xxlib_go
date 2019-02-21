#include "../test_uv_3/xx_uv_msg.h"
#include <unordered_set>
#include <chrono>
#include <iostream>
#include <thread>

int main() {
	UvLoop loop;
	std::unordered_set<std::shared_ptr<UvTcpMsgPeer>> peers;
	auto listener = loop.CreateListener<UvTcpMsgListener>("0.0.0.0", 12345);
	assert(listener);
	listener->OnAccept = [&peers](std::shared_ptr<UvTcpBasePeer>&& peer_) {
		auto peer = std::static_pointer_cast<UvTcpMsgPeer>(peer_);
		peer->OnReceiveRequest = [peer_w = std::weak_ptr<UvTcpMsgPeer>(peer)](int const& serial, UvTcpMsgPeer::MsgType&& msg)->int {
			return peer_w.lock()->SendResponse(serial, msg);
		};
		peer->OnDisconnect = [&peers, peer_w = std::weak_ptr<UvTcpMsgPeer>(peer)]{
			peers.erase(peer_w.lock());
		};
		peers.insert(std::move(peer));
	};
	loop.Run();
	std::cout << "server end.\n";
	return 0;
}
