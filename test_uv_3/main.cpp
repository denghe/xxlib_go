#include "xx_uv_cpp.h"
#include <boost/coroutine2/all.hpp>
#include <iostream>

using Coroutine = boost::coroutines2::coroutine<void>::pull_type;
using CoroutineFunc = boost::coroutines2::coroutine<void>::push_type;

void TestCor1(Coroutine& yield) {
	for (size_t i = 0; i < 100; i++) {
		std::cout << i;
		yield();
	}
}

struct Coroutines {
	std::vector<CoroutineFunc> cors;
	inline void Add(CoroutineFunc&& cf) {
		cf();
		if (cf) {
			cors.push_back(std::move(cf));
		}
	}
	inline void RunAdd(CoroutineFunc&& cf) {
		cors.push_back(std::move(cf));
	}
	inline void RemoveAt(size_t const& idx) {
		if (idx + 1 < cors.size()) {
			cors[idx] = std::move(cors[cors.size() - 1]);
		}
		cors.pop_back();
	}
	void RunOnce() {
		if (cors.size()) {
			for (decltype(auto) i = cors.size() - 1; i != (size_t)-1; --i) {
				decltype(auto) cor = cors[i];
				if (cor) {
					cor();
				}
				else {
					RemoveAt(i);
				}
			}
		}
	}
};

int main() {
	UvLoop loop;
	Coroutines cs;
	cs.Add(CoroutineFunc(TestCor1));
	cs.Add(CoroutineFunc(TestCor1));
	cs.RunOnce();
	auto timer = loop.CreateTimer(0, 1000 / 61, [&] { cs.RunOnce(); });
	loop.Run();
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
