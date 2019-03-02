#include "xx_uv.h"
#include <unordered_set>

struct EchoLoop : xx::UvLoop {
	std::unordered_set<xx::UvTcpBasePeer_s> peers;
};
struct EchoPeer : xx::UvTcpBasePeer {
	inline int Unpack(uint8_t* const& buf, uint32_t const& len) noexcept override {
		return Send(buf, len);
	}

};
struct EchoListener : xx::UvTcpBaseListener {
	EchoLoop* loop;
	inline virtual xx::UvTcpBasePeer_s CreatePeer() noexcept override {
		return std::make_shared<EchoPeer>();
	}
	inline virtual void Accept(xx::UvTcpBasePeer_s&& peer) noexcept override {
		peer->OnDisconnect = [loop = this->loop, peer_w = xx::UvTcpBasePeer_w(peer)] {
			loop->peers.erase(peer_w.lock());
		};
		loop->peers.insert(std::move(peer));
	}
};
int main() {
	EchoLoop loop;
	auto listener = loop.CreateTcpListener<EchoListener>("0.0.0.0", 12345);
	listener->loop = &loop;
	loop.Run();
	return 0;
}
