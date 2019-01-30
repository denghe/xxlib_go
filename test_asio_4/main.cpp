#include "asio.hpp"
#include <iostream>
#include <functional>

std::mutex coutMutex;
template<typename ...Args>
void Cout(Args const& ... args) {
	std::lock_guard<std::mutex> lg_mutex(coutMutex);
	std::initializer_list<int> n{ ((std::cout << args), 0)... };
	(void)(n);
}
template<typename ...Args>
void CoutT(Args const& ... args) {
	Cout("[", std::this_thread::get_id(), "] ", args...);
}

int main(int argc, char* argv[])
{
	return 0;
}
