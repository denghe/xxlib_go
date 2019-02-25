#include "xx_uv_msg.h"
#include "../gens/output/PKG_class.h"
#include <iostream>

int main() {
	PKG::AllTypesRegister();
	std::cout << PKG::PkgGenMd5::value << std::endl;

	auto o = std::make_shared<PKG::Generic::Error>();
	std::cout << o << std::endl;

	xx::BBuffer bb;
	bb.WriteRoot(o);
	std::cout << bb << std::endl;
	
	return 0;
}
