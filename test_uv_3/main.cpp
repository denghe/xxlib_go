//#include "xx_uv_cpp.h"
//#include "xx_uv_cpp_echo.h"
//#include "xx_uv_cpp_package.h"
//
//int main() {
//	TestEcho();
//	return 0;
//}


#include "xx_bbuffer.h"
#include <iostream>

struct Node : BItem {
	std::shared_ptr<Node> parent;
	std::shared_ptr<std::string> name;

	inline virtual uint16_t GetTypeId() const noexcept override { return 3; }
	inline virtual void ToBBuffer(BBuffer& bb) const noexcept override {
		bb.Write(this->parent);
		bb.Write(this->name);
	}
	inline virtual int FromBBuffer(BBuffer& bb) noexcept override {
		if (int r = bb.Read(this->parent)) return r;
		if (int r = bb.Read(this->name)) return r;
		return 0;
	}
};

int main() {
	UvLoopEx loop;
	BBuffer bb(loop);

	auto node = std::make_shared<Node>();
	node->parent = node;
	node->name = std::make_shared<std::string>("asdf");

	bb.WriteRoot(node);
	std::cout << bb.ToString() << std::endl;

	return 0;
}
