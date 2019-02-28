#include "xx_uv.h"
#include "../gens/output/PKG_class.h"
#include <iostream>
#include <unordered_set>

namespace xx {
	struct RouterListener;
	using RouterListener_s = std::shared_ptr<RouterListener>;
	struct RouterPeer;
	using RouterPeer_s = std::shared_ptr<RouterPeer>;
	struct RouterDialer;
	using RouterDialer_s = std::shared_ptr<RouterDialer>;

	struct Router : UvLoop {
		BBuffer readBB;			// for replace buf memory decode package
		int addr = 0;			// for gen client peer addr
		std::unordered_map<int, RouterPeer_s> clientPeers;
		std::unordered_map<int, RouterPeer_s> serverPeers;
		std::shared_ptr<RouterListener> listener;
		UvTimer_s timer;	// for auto dial
		std::unordered_map<int, RouterDialer_s> dialers;
		int RegisterDialer(int const& addr, std::string&& ip, int const& port) noexcept;

		Router();
		~Router() {
			readBB.Reset();
		}
	};

	// 包结构: addr(4), data( serial + pkg )
	struct RouterPeer : UvTcpBasePeer {
		BBuffer* bb = nullptr;										// for replace buf memory decode package
		std::unordered_map<int, RouterPeer_s>* peers = nullptr;	// for find target peer
		int addr = 0;

		RouterPeer() = default;
		RouterPeer(RouterPeer const&) = delete;
		RouterPeer& operator=(RouterPeer const&) = delete;

		virtual int HandlePack(uint8_t* const& recvBuf, uint32_t const& recvLen) noexcept {
			bb->Reset(recvBuf, recvLen, 0);

			decltype(this->addr) addr = 0;
			if (int r = bb->ReadFixed(addr)) return r;

			auto iter = peers->find(addr);
			if (iter != peers->end()) {
				memcpy(recvBuf, &this->addr, sizeof(this->addr));	// replace addr
				return iter->second->Send(recvBuf, recvLen);
			}
			// todo: else 地址不存在的回执通知?
			return 0;
		}
	};

	struct RouterDialer : UvTcpDialer {
		using UvTcpDialer::UvTcpDialer;
		Router* router = nullptr;
		int addr = 0;
		std::string ip;
		int port = 0;
		inline int Dial() {
			return this->UvTcpDialer::Dial(ip, port, 2000);
		}

		inline virtual std::shared_ptr<UvTcpBasePeer> CreatePeer() noexcept {
			return std::make_shared<RouterPeer>();
		}
		inline virtual void Connect() noexcept {
			auto peer = std::move(Peer<RouterPeer>());
			peer->addr = addr;
			peer->peers = &router->serverPeers;
			peer->bb = &router->readBB;
			router->serverPeers[peer->addr] = std::move(peer);
		}
	};

	struct RouterListener : UvTcpBaseListener {
		Router* router = nullptr;

		RouterListener() = default;
		RouterListener(RouterListener const&) = delete;
		RouterListener& operator=(RouterListener const&) = delete;

		inline virtual std::shared_ptr<UvTcpBasePeer> CreatePeer() noexcept override {
			return std::make_shared<RouterPeer>();
		}
		inline virtual void Accept(std::shared_ptr<UvTcpBasePeer>&& peer_) noexcept override {
			auto peer = std::move(xx::As<RouterPeer>(peer_));
			peer->addr = ++router->addr;
			peer->peers = &router->clientPeers;
			peer->bb = &router->readBB;
			router->clientPeers[peer->addr] = std::move(peer);
		};
	};

	inline Router::Router()
		: UvLoop() {
		listener = CreateTcpListener<RouterListener>("0.0.0.0", 10000);
		if (listener) {
			listener->router = this;
			std::cout << "router started...";
		}

		timer = CreateTimer<>(100, 500, [this] {
			for (decltype(auto) kv : dialers) {
				if (!serverPeers[kv.first] && !kv.second->State()) {
					kv.second->Dial();
				}
			}
		});
	}

	inline int Router::RegisterDialer(int const& addr, std::string&& ip, int const& port) noexcept {
		if (dialers.find(addr) == dialers.end()) return -1;
		auto dialer = CreateTcpDialer<RouterDialer>();
		if (!dialer) return -2;
		dialer->addr = addr;
		dialer->ip = std::move(ip);
		dialer->port = port;
		dialers[addr] = dialer;
		return 0;
	}
}

// todo: UvTcpRouterListener UvTcpRouterListenerPeer UvTcpRouterListenerPeerPeer  UvTcpRouterDialer UvTcpRouterDialerPeer UvTcpRouterDialerPeerDialer UvTcpRouterDialerPeerDialerPeer
// UvTcpRouterListener: 本身自己管理 Accept 后的 UvTcpRouterListenerPeer, 然后提供在收到握手协议之后的相关事件 UvTcpRouterListenerPeerPeer
// UvTcpRouterDialer: 本身自己管理 Connect 后的 UvTcpRouterDialerPeer, 用户与拨号成功后继续访问 UvTcpRouterDialerPeer 创建虚拟拨号器
// UvTcpRouterDialerPeerDialer: 与 UvTcpDialer 用法差不多, 虚拟拨号, Connect 后得到 UvTcpRouterListenerPeerPeer

// todo: 将当前 UvTcpPeer 里面路由相关移除
namespace xx {
	struct UvTcpRouterListener;
	struct UvTcpRouterListenerPeer : UvTcpBasePeer {
		size_t indexAtContainer = -1;
		UvTcpRouterListener* listener = nullptr;

		UvTcpRouterListenerPeer() = default;
		UvTcpRouterListenerPeer(UvTcpRouterListenerPeer const&) = delete;
		UvTcpRouterListenerPeer& operator=(UvTcpRouterListenerPeer const&) = delete;

		virtual int HandlePack(uint8_t* const& recvBuf, uint32_t const& recvLen) noexcept override {
			// todo
			return 0;
		}
		virtual void Dispose(bool callback = true) noexcept override;
	};
	using UvTcpRouterListenerPeer_s = std::shared_ptr<UvTcpRouterListenerPeer>;
	using UvTcpRouterListenerPeer_w = std::weak_ptr<UvTcpRouterListenerPeer>;

	struct UvTcpRouterListener : UvTcpBaseListener {
		xx::List<UvTcpRouterListenerPeer_s> peers;

		UvTcpRouterListener() = default;
		UvTcpRouterListener(UvTcpRouterListener const&) = delete;
		UvTcpRouterListener& operator=(UvTcpRouterListener const&) = delete;

		inline virtual std::shared_ptr<UvTcpBasePeer> CreatePeer() noexcept override {
			return std::make_shared<UvTcpRouterListenerPeer>();
		}
		inline virtual void Accept(std::shared_ptr<UvTcpBasePeer>&& peer_) noexcept override {
			auto peer = std::move(xx::As<UvTcpRouterListenerPeer>(peer_));
			peer->listener = this;
			peer->OnDisconnect = [this, peer_w = UvTcpRouterListenerPeer_w(peer)]{
				auto peer = peer_w.lock();

				// todo: 

				peer->listener->peers[peer->listener->peers.len - 1]->indexAtContainer = peer->indexAtContainer;
				peer->listener->peers.SwapRemoveAt(peer->indexAtContainer);
			};
			peer->indexAtContainer = peers.len;
			peers.Add(std::move(peer));
		}
	};

	inline void UvTcpRouterListenerPeer::Dispose(bool callback) noexcept {
		// todo: 级联关闭
		if (!uvTcp) return;
		// todo: Dispose all UvTcpRouterListenerPeerPeer
		listener->peers[listener->peers.len - 1]->indexAtContainer = indexAtContainer;
		listener->peers.SwapRemoveAt(indexAtContainer);
		this->UvTcpBasePeer::Dispose(callback);
	}

}

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

void RunRouter() {

}

int main() {
	return 0;
}































//#define var decltype(auto)

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
