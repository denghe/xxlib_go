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

int main() {

	BBuffer bb;
	bb.Write(123);
	bb.Write(123);
	bb.Write(123);
	for (size_t i = 0; i < bb.len; i++)
	{
		std::cout << (int)bb[i] << " ";
	}

	return 0;
}
