#include <libgo/libgo.h>
#include <chrono>
#include <iostream>

co_timer timer(std::chrono::milliseconds(1), &co_sched);
size_t n = 0;
void timerCB() {
	++n;
	timer.ExpireAt(std::chrono::milliseconds(1), timerCB);
}
int main()
{
	timerCB();

	co_timer timer2(std::chrono::nanoseconds(1), &co_sched);
	//co_timer timer2(std::chrono::milliseconds(1), &co_sched);
	timer2.ExpireAt(std::chrono::seconds(1), [] 
	{
		std::cout << n << std::endl;
	});

	go[]{
		std::cout << "xx\n";
	};
	co_sched.Start();
	return 0;
}
