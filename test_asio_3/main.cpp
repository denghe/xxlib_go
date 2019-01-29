﻿// https://www.gamedev.net/blogs/entry/2249317-a-guide-to-getting-started-with-boostasio/
// 5a

#define var decltype(auto)

#include "asio/io_service.hpp"
#include "asio/basic_waitable_timer.hpp"
#include <iostream>
#include <memory>
#include <thread>
#include <mutex>
#include <chrono>

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

void WorkerThread(asio::io_service& io_service) {
	CoutT("thread start\n");

	while (true) {
		try {
			asio::error_code ec;
			io_service.run(ec);
			if (ec) {
				CoutT("asio error: ", ec, "\n");
			}
			break;
		}
		catch (std::exception& ex) {
			CoutT("exception: ", ex.what(), "\n");
		}
	}

	CoutT("thread finish\n");
}

void TimerHandler(asio::error_code const& err) {
	if (err) {
		CoutT("error: ", err);
	}
	else {
		CoutT("timerHandler.\n");
	}
}

int main(int argc, char* argv[])
{
	asio::io_service ios;
	try {
		var work = std::make_shared<asio::io_service::work>(ios);

		Cout("Press [return] to exit.\n");

		std::vector<std::thread> worker_threads;
		for (size_t i = 0; i < 2; i++)
		{
			worker_threads.emplace_back(std::bind(&::WorkerThread, std::ref(ios)));
		}

		asio::basic_waitable_timer<std::chrono::system_clock> timer(ios);
		timer.expires_from_now(std::chrono::seconds(1));
		timer.async_wait(TimerHandler);

		std::cin.get();
		ios.stop();

		for (var t : worker_threads) {
			t.join();
		}
	}
	catch (std::exception& e) {
		Cout(e.what(), "\n");
	}
	return 0;
}
