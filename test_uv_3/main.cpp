#include "xx_uv.h"
#include "xx_uv_stackless.h"
#include <iostream>


int main() {
	UvLoopStackless loop(61);
	struct Ctx1 {
		std::shared_ptr<UvTcpClient> client;
		std::shared_ptr<UvTcpPeer> peer;
		std::chrono::system_clock::time_point t;
		std::vector<Buffer> recvs;
		int count = 0;
	};
	loop.Add([&, zs = std::make_shared<Ctx1>()](int const& lineNumber) {
		COR_BEGIN
			zs->client = loop.CreateClient<UvTcpClient>();
		LabConnect:
		//	if (++zs->count > 3) goto LabEnd;
			std::cout << "connecting...\n";
			zs->client->Connect("127.0.0.1", 12345);
		//	zs->t = std::chrono::system_clock::now() + std::chrono::seconds(2);
		//	while (!zs->client->peer) {
		//		COR_YIELD
		//		if (std::chrono::system_clock::now() > zs->t) {		// timeout check
		//			std::cout << "timeout. retry\n";
		//			goto LabConnect;
		//		}
		//	}
		//	std::cout << "connected.\n";
		//	zs->peer = std::move(std::static_pointer_cast<UvTcpPeer>(zs->client->peer));
		//	zs->peer->OnReceivePack = [wzs = std::weak_ptr<Ctx1>(zs)](uint8_t const* const& buf, uint32_t const& len) {
		//		if (auto zs = wzs.lock()) {
		//			Buffer b(len);
		//			b.Append(buf, len);
		//			zs->recvs.push_back(std::move(b));
		//		}
		//		return 0;
		//	};
		//	zs->recvs.clear();

		//LabSend:
		//	std::cout << "send 'asdf'\n";
		//	zs->peer->SendPack((uint8_t*)"asdf", 4);

		//	std::cout << "wait echo & check state\n";
		//	while (!zs->peer->Disposed()) {
		//		COR_YIELD
		//		if (zs->recvs.size()) {
		//			for (decltype(auto) b : zs->recvs) {
		//				std::cout << std::string((char*)b.buf, b.len) << std::endl;
		//			}
		//			zs->recvs.clear();
		//			goto LabSend;
		//		}
		//	}
		//	std::cout << "disconnected. retry\n";
		//	goto LabConnect;
		LabEnd:;
		COR_END
	});
	loop.Run();
	std::cout << "end.\n";
	std::cin.get();
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
