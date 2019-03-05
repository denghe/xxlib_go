#include "xx_object.h"
#include <iostream>
struct Foo {
	Foo() {
		std::cout << "Foo()\n";
	}
	~Foo() {
		std::cout << "~Foo()\n";
	}
};
struct FooEx : Foo {
	FooEx() {
		throw - 1;
	}
};

int main(int argc, char* argv[]) {
	auto foo = xx::TryMake<FooEx>();
	return 0;
}
