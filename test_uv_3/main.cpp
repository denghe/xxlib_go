﻿#include "xx_uv_msg.h"
#include <unordered_set>
#include <chrono>
#include <iostream>
#include <thread>

struct MyClient : xx::UvTcpMsgClient {
	using UvTcpMsgClient::UvTcpMsgClient;
	int counter = 0;
	std::chrono::time_point<std::chrono::system_clock> t = std::chrono::system_clock::now();
	int HandleMsg(xx::UvTcpMsgPeer::MsgType&& msg) {
		if (!msg) return -1;
		//auto bb = std::dynamic_pointer_cast<BBuffer>(msg);
		//if (!bb) return -1;
		//std::cout << bb->GetTypeId() << std::endl;
		//std::cout << bb->ToString() << std::endl;
		if (++counter > 100000) {
			std::cout << double(std::chrono::nanoseconds(std::chrono::system_clock::now() - t).count()) / 1000000000 << std::endl;
			return -1;
		}
		return Peer()->SendRequest(msg, [this](xx::UvTcpMsgPeer::MsgType&&msg) {
			return HandleMsg(std::move(msg));
		}, 1000);
	}
};

void RunServer() {
	xx::UvLoop loop;
	std::unordered_set<std::shared_ptr<xx::UvTcpMsgPeer>> peers;
	auto listener = loop.CreateListener<xx::UvTcpMsgListener>("0.0.0.0", 12345);
	assert(listener);
	listener->OnAccept = [&peers](std::shared_ptr<xx::UvTcpBasePeer>&& peer_) {
		auto peer = std::static_pointer_cast<xx::UvTcpMsgPeer>(peer_);
		peer->OnReceiveRequest = [peer_w = std::weak_ptr<xx::UvTcpMsgPeer>(peer)](int const& serial, xx::UvTcpMsgPeer::MsgType&& msg)->int {
			return peer_w.lock()->SendResponse(serial, msg);
		};
		peer->OnDisconnect = [&peers, peer_w = std::weak_ptr<xx::UvTcpMsgPeer>(peer)]{
			peers.erase(peer_w.lock());
		};
		peers.insert(std::move(peer));
	};
	loop.Run();
	std::cout << "server end.\n";
}

int main() {
	std::thread t1([] { RunServer(); });
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	xx::UvLoop loop;
	auto client = loop.CreateClient<MyClient>();
	assert(client);
	client->OnConnect = [client_w = std::weak_ptr<MyClient>(client)]{
		auto msg = std::make_shared<xx::BBuffer>();
		msg->Write(1u, 2u, 3u, 4u, 5u);
		client_w.lock()->Peer()->SendRequest(msg, [client_w] (xx::UvTcpMsgPeer::MsgType&&msg) {
			return client_w.lock()->HandleMsg(std::move(msg));
		}, 0);
	};
	client->OnTimeout = [] {
		std::cout << "dial timeout.\n";
	};
	client->Dial("127.0.0.1", 12345, 2000);
	loop.Run();
	std::cout << "client end.\n";
	return 0;
}























//#include "xx_uv_cpp.h"
//#include "xx_uv_cpp_echo.h"
//#include "xx_uv_cpp_package.h"
//
//int main() {
//	TestEcho();
//	return 0;
//}
//
////struct Pos {
////	double x = 0, y = 0;
////};
////template<>
////struct BFuncs<Pos, void> {
////	static inline void WriteTo(BBuffer& bb, Pos const& in) noexcept {
////		bb.Write(in.x, in.y);
////	}
////	static inline int ReadFrom(BBuffer& bb, Pos& out) noexcept {
////		return bb.Read(out.x, out.y);
////	}
////};
//
//#include "xx_bbuffer.h"
//#include <iostream>
//
//struct Node : BObject {
//	int indexAtContainer;
//	std::weak_ptr<Node> parent;
//	std::vector<std::weak_ptr<Node>> childs;
//#pragma region
//	inline virtual uint16_t GetTypeId() const noexcept override {
//		return 3;
//	}
//	inline virtual void ToBBuffer(BBuffer& bb) const noexcept override {
//		bb.Write(this->indexAtContainer, this->parent, this->childs);
//	}
//	inline virtual int FromBBuffer(BBuffer& bb) noexcept override {
//		return bb.Read(this->indexAtContainer, this->parent, this->childs);
//	}
//#pragma endregion
//};
//
//struct Container : BObject {
//	std::vector<std::shared_ptr<Node>> nodes;
//	std::weak_ptr<Node> node;
//#pragma region
//	inline virtual uint16_t GetTypeId() const noexcept override {
//		return 4;
//	}
//	inline virtual void ToBBuffer(BBuffer& bb) const noexcept override {
//		bb.Write(this->nodes, this->node);
//	}
//	inline virtual int FromBBuffer(BBuffer& bb) noexcept override {
//		return bb.Read(this->nodes, this->node);
//	}
//#pragma endregion
//};
//
//int main() {
//	BBuffer::Register<Node>(3);
//	BBuffer::Register<Container>(4);
//
//	auto c = std::make_shared<Container>();
//	auto n = std::make_shared<Node>();
//	n->indexAtContainer = c->nodes.size();
//	c->nodes.push_back(std::move(n));
//	n = std::make_shared<Node>();
//	n->indexAtContainer = c->nodes.size();
//	c->nodes.push_back(std::move(n));
//	c->node = c->nodes[0];
//	c->node.lock()->parent = c->node;
//	c->node.lock()->childs.push_back(c->node);
//	c->node.lock()->childs.push_back(c->nodes[1]);
//
//	BBuffer bb;
//	bb.WriteRoot(c);
//	std::cout << bb.ToString() << std::endl;
//
//	std::shared_ptr<Container> tmp;
//	int r = bb.ReadRoot(tmp);
//	assert(!r);
//	auto c2 = std::dynamic_pointer_cast<Container>(tmp);
//	assert(c2);
//	std::cout << c2->nodes.size()
//		<< " " << (c2->node.lock() == c2->nodes[0])
//		<< " " << (c2->node.lock() == c2->node.lock()->parent.lock())
//		<< " " << c2->node.lock()->childs.size()
//		<< " " << (c2->node.lock()->childs[0].lock() == c2->node.lock())
//		<< " " << (c2->nodes[1] == c2->node.lock()->childs[1].lock())
//		<< std::endl;
//
//	return 0;
//}
