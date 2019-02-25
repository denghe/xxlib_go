#include "xx_uv_msg.h"
#include "../gens/output/PKG_class.h"
#include <iostream>

int main() {
	PKG::AllTypesRegister();
	std::cout << PKG::PkgGenMd5::value << std::endl;

	auto o = std::make_shared<PKG::Foo>();
	o->parent = o;
	o->childs = o->childs->Create();
	o->childs->Add(o);
	std::cout << o << std::endl;

	xx::BBuffer bb;
	bb.WriteRoot(o);
	std::cout << bb << std::endl;

	auto o2 = PKG::Foo::Create();
	int r = bb.ReadRoot(o2);
	std::cout << r << std::endl;
	std::cout << o2 << std::endl;
	
	return 0;
}
