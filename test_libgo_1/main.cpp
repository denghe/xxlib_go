#include <libgo/libgo.h>
#include <chrono>
#include <iostream>
#include <thread>

int main()
{
	go[]
	{
		std::cout << "go1\n";
		auto t = std::chrono::system_clock::now();
		for (int i = 0; i < 10000; ++i) {
			std::this_thread::sleep_for(std::chrono::nanoseconds(1));
		}
		
		std::cout << (double(std::chrono::nanoseconds(std::chrono::system_clock::now() - t).count()) / 1000000000) << std::endl;
	};
	go[]
	{
		std::cout << "go2\n";
		for (int i = 0; i < 10000; ++i) {
			std::this_thread::sleep_for(std::chrono::seconds(1));
			std::cout << ".";
			//co_yield;
		}
	};
	co_sched.Start();
	return 0;
}
