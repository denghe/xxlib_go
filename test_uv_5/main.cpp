#include "xx_uv.h"
#include "../gens/output/PKG_class.h"
#include <iostream>
#include <unordered_set>

#define var decltype(auto)

void RunServer1() {
	xx::UvLoop loop;
	std::unordered_set<xx::UvTcpPeer_s> peers;
	auto listener = loop.CreateTcpListener<xx::UvTcpListener>("0.0.0.0", 10001);
	assert(listener);
	listener->OnAccept = [&peers](xx::UvTcpPeer_s&& peer) {
		peer->OnReceiveRouteRequest = [peer_w = xx::UvTcpPeer_w(peer)](int const& addr, int const& serial, xx::Object_s&& msg)->int {
			return peer_w.lock()->SendResponse(serial, msg, addr);
		};
		peer->OnDisconnect = [&peers, peer_w = xx::UvTcpPeer_w(peer)]{
			peers.erase(peer_w.lock());
		};
		peers.insert(std::move(peer));
	};
	loop.Run();
	std::cout << "server end.\n";
}

void RunServer2() {
	xx::UvLoop loop;
	std::unordered_set<xx::UvTcpPeer_s> peers;
	auto listener = loop.CreateTcpListener<xx::UvTcpListener>("0.0.0.0", 10002);
	assert(listener);
	listener->OnAccept = [&peers](xx::UvTcpPeer_s&& peer) {
		peer->OnReceiveRouteRequest = [peer_w = xx::UvTcpPeer_w(peer)](int const& addr, int const& serial, xx::Object_s&& msg)->int {
			return peer_w.lock()->SendResponse(serial, msg, addr);
		};
		peer->OnDisconnect = [&peers, peer_w = xx::UvTcpPeer_w(peer)]{
			peers.erase(peer_w.lock());
		};
		peers.insert(std::move(peer));
	};
	loop.Run();
	std::cout << "server end.\n";
}



struct RouterPeer : xx::UvTcpPeer {
	using BaseType = xx::UvTcpPeer;
	using BaseType::BaseType;
	int peerId = 0;
};
using RouterPeer_s = std::shared_ptr<RouterPeer>;
using RouterPeer_w = std::weak_ptr<RouterPeer>;

struct RouterListener : xx::UvTcpListener {
	using BaseType = xx::UvTcpListener;
	using BaseType::BaseType;
	inline virtual std::shared_ptr<xx::UvTcpPeer> CreatePeer() noexcept override {
		return std::make_shared<RouterPeer>();
	}
};
using RouterListener_s = std::shared_ptr<RouterListener>;
using RouterListener_w = std::weak_ptr<RouterListener>;

void RunRouter() {
	xx::UvLoop loop;

	int peerId = 0;	// for gen connect id
	std::unordered_map<int, RouterPeer_s> clientPeers;
	std::unordered_map<int, xx::UvTcpPeer_s> serverPeers;

	auto listener = loop.CreateTcpListener<RouterListener>("0.0.0.0", 10000);
	assert(listener);
	listener->OnAccept = [&](xx::UvTcpPeer_s&& peer_) {
		auto peer = std::static_pointer_cast<RouterPeer>(peer_);
		peer->OnReceiveRoute = [&, peer_w = RouterPeer_w(peer)](int const& addr, uint8_t* const& recvBuf, uint32_t const& recvLen)->int {
			auto iter = serverPeers.find(addr);
			if (iter == serverPeers.end()) return -1;		// todo: 返回已断开的控制指令回应
			memcpy(recvBuf, &peer_w.lock()->peerId, sizeof(peer_w.lock()->peerId));		// 将地址替换为 client peerId 以方便 server 回复时携带以定位到 client peer
			return iter->second->Send(recvBuf, recvLen);
		};
		peer->OnDisconnect = [&, peer_w = RouterPeer_w(peer)]{
			auto peer = peer_w.lock();
			assert(peer);
			clientPeers.erase(peer->peerId);
		};
		peer->peerId = ++peerId;
		clientPeers[peerId] = std::move(peer);
	};

	auto dialer1 = loop.CreateTcpDialer<>();
	assert(dialer1);
	dialer1->OnConnect = [&] {
		assert(!serverPeers[1]);
		dialer1->peer->OnReceiveRoute = [&](int const& addr, uint8_t* const& recvBuf, uint32_t const& recvLen)->int {
			auto iter = clientPeers.find(addr);
			if (iter == clientPeers.end()) return -1;		// todo: 返回已断开的控制指令回应
			int serverId = 1;
			memcpy(recvBuf, &serverId, sizeof(serverId));		// 将地址替换为 server peerId 以方便 client 收到时知道是哪个 server 发出的
			return iter->second->Send(recvBuf, recvLen);
		};
		serverPeers[1] = std::move(dialer1->peer);
	};

	auto dialer2 = loop.CreateTcpDialer<>();
	assert(dialer2);
	dialer2->OnConnect = [&] {
		assert(!serverPeers[2]);
		dialer2->peer->OnReceiveRoute = [&](int const& addr, uint8_t* const& recvBuf, uint32_t const& recvLen)->int {
			auto iter = clientPeers.find(addr);
			if (iter == clientPeers.end()) return -1;		// todo: 返回已断开的控制指令回应
			int serverId = 1;
			memcpy(recvBuf, &serverId, sizeof(serverId));		// 将地址替换为 server peerId 以方便 client 收到时知道是哪个 server 发出的
			return iter->second->Send(recvBuf, recvLen);
		};
		serverPeers[2] = std::move(dialer2->peer);
	};

	auto timer = loop.CreateTimer(100, 100, [&] {
		if (!serverPeers[1] && !dialer1->State()) {
			dialer1->Dial("127.0.0.1", 10001);
		}
		if (!serverPeers[2] && !dialer2->State()) {
			dialer2->Dial("127.0.0.1", 10002);
		}
	});
	assert(timer);

	loop.Run();
	std::cout << "server end.\n";
}

int main() {
	return 0;
}

































//struct GameEnv {
//	std::unordered_map<int, PKG::Player_s> players;
//	PKG::Scene_s scene;
//};
//
//int main() {
//	PKG::AllTypesRegister();
//
//	GameEnv env;
//	xx::MakeShared(env.scene);
//	xx::MakeShared(env.scene->monsters);
//	xx::MakeShared(env.scene->players);
//
//	var p1 = PKG::Player::MakeShared();
//	var p2 = PKG::Player::MakeShared();
//	var m1 = PKG::Monster::MakeShared();
//	var m2 = PKG::Monster::MakeShared();
//
//	p1->id = 123;
//	xx::MakeShared(p1->token, "asdf");
//	p1->target = m1;
//
//	p2->id = 234;
//	xx::MakeShared(p1->token, "qwer");
//	p2->target = m2;
//
//	env.players[p1->id] = p1;
//	env.players[p2->id] = p2;
//
//	env.scene->monsters->Add(m1, m2);
//	env.scene->players->Add(p1, p2);
//
//	std::cout << env.scene << std::endl;
//
//	xx::BBuffer bb;
//	{
//		var sync = PKG::Sync::MakeShared();
//		xx::MakeShared(sync->players);
//		for (var player_w : *env.scene->players) {
//			if (var player = player_w.lock()) {
//				sync->players->Add(player);
//			}
//		}
//		sync->scene = env.scene;
//		bb.WriteRoot(sync);
//	}
//	std::cout << bb << std::endl;
//
//	//env.players
//	return 0;
//}
//

























//int main() {
//	PKG::AllTypesRegister();
//	std::cout << PKG::PkgGenMd5::value << std::endl;
//
//	auto o = std::make_shared<PKG::Foo>();
//	o->parent = o;
//	o->childs = o->childs->Create();
//	o->childs->Add(o);
//	std::cout << o << std::endl;
//
//	xx::BBuffer bb;
//	bb.WriteRoot(o);
//	std::cout << bb << std::endl;
//
//	auto o2 = PKG::Foo::Create();
//	int r = bb.ReadRoot(o2);
//	std::cout << r << std::endl;
//	std::cout << o2 << std::endl;
//	
//	return 0;
//}
