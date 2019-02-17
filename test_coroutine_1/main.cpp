#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <boost/coroutine2/all.hpp>

using Coroutine = boost::coroutines2::coroutine<void>::pull_type;
using CoroutineFunc = boost::coroutines2::coroutine<void>::push_type;

void TestCor1(Coroutine& yield) {
	for (size_t i = 0; i < 1000; i++) {
		yield();
	}
}

struct Coroutines {
	std::vector<CoroutineFunc> cors;
	inline void Add(CoroutineFunc&& cf) {
		cors.push_back(std::move(cf));
	}
	inline void RemoveAt(size_t const& idx) {
		if (idx + 1 < cors.size()) {
			cors[idx] = std::move(cors[cors.size() - 1]);
		}
		cors.pop_back();
	}
	void Run() {
		while (cors.size()) {
			for (decltype(auto) i = cors.size() - 1; i != (size_t)-1; --i) {
				cors[i]();
				if (!cors[i]) {
					RemoveAt(i);
				}
			}
			std::this_thread::sleep_for(std::chrono::nanoseconds(1));
		}
	}
};

int main(int argc, char* argv[])
{
	decltype(auto) t = std::chrono::system_clock::now();

	Coroutines cs;
	cs.Add(CoroutineFunc(TestCor1));
	cs.Add(CoroutineFunc(TestCor1));
	cs.Run();

	std::cout << double(std::chrono::nanoseconds(std::chrono::system_clock::now() - t).count()) / 1000000000;

	getchar();
	return 0;
}
