﻿//#include "xx_uv_cpp.h"
//#include "xx_uv_cpp_echo.h"
//#include "xx_uv_cpp_package.h"
//
//int main() {
//	TestEcho();
//	return 0;
//}


#include "xx_bbuffer.h"
#include <iostream>

struct Pos {
	double x = 0, y = 0;
};
template<>
struct BFuncs<Pos, void> {
	static inline void WriteTo(BBuffer& bb, Pos const& in) noexcept {
		bb.Write(in.x, in.y);
	}
	static inline int ReadFrom(BBuffer& bb, Pos& out) noexcept {
		return bb.Read(out.x, out.y);
	}
};
struct Node : BObject {
	std::shared_ptr<Node> parent;
	std::shared_ptr<std::string> name;
	Pos pos;
	int age = 0;

	inline virtual uint16_t GetTypeId() const noexcept override { 
		return 3;
	}
	inline virtual void ToBBuffer(BBuffer& bb) const noexcept override {
		bb.Write(this->parent, this->name, this->pos, this->age);
	}
	inline virtual int FromBBuffer(BBuffer& bb) noexcept override {
		return bb.Read(this->parent, this->name, this->pos, this->age);
	}
};

int main() {
	BBuffer bb;
	BBuffer::Register(3, []()->std::shared_ptr<BObject> { return std::make_shared<Node>(); });

	auto node = std::make_shared<Node>();
	node->parent = node;
	node->name = std::make_shared<std::string>("asdf");
	node->pos = Pos{ 23, 45 };
	node->age = 3;

	bb.WriteRoot(node);
	std::cout << bb.ToString() << std::endl;

	std::shared_ptr<BObject> n2;
	int r = bb.ReadRoot(n2);
	assert(!r);
	auto node2 = std::dynamic_pointer_cast<Node>(n2);
	assert(node2);
	std::cout << node2->age << " " << node2->pos.x << " " << node2->pos.y << " " << &*node2 << " " << &*node2->parent;

	return 0;
}
