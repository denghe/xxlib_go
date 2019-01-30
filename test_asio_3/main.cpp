// https://www.gamedev.net/blogs/entry/2249317-a-guide-to-getting-started-with-boostasio/
// 7c

#define var decltype(auto)

#include "asio/io_service.hpp"
#include "asio/basic_waitable_timer.hpp"
#include "asio/strand.hpp"
#include "asio/ip/tcp.hpp"
#include "asio/write.hpp"
#include <iostream>
#include <memory>
#include <thread>
#include <mutex>
#include <chrono>
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

struct Peer : std::enable_shared_from_this<Peer> {
	asio::ip::tcp::socket&& socket;
	std::array<char, 512> buf;
	Peer(asio::ip::tcp::socket&& socket)
		: socket(std::move(socket))
	{
	}
	void BeginRead() {
		socket.async_read_some(asio::buffer(buf), [this, self = shared_from_this()](asio::error_code const& ec, std::size_t const& length) {
			if (!ec) {
				BeginWrite(length);	// echo
			}
			else {
				CoutT("Peer read error: ", ec, "\n");
			}
		});
	}
	void BeginWrite(std::size_t const& length) {
		asio::async_write(socket, asio::buffer(buf, length), [this, self = shared_from_this()](asio::error_code const& ec, std::size_t length) {
			if (!ec) {
				BeginRead();
			}
			else {
				CoutT("Peer write error: ", ec, "\n");
			}
		});
	}
};

struct Acceptor {
	asio::ip::tcp::acceptor acceptor;
	Acceptor(asio::io_context& ios, uint16_t port)
		: acceptor(ios, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
	{
		BeginAccept();
	}
	void BeginAccept() {
		acceptor.async_accept([this](asio::error_code const& ec, asio::ip::tcp::socket& socket) {
			if (!ec) {
				CoutT("Acceptor accepted!\n");
				std::make_shared<Peer>(std::move(socket))->BeginRead();
			}
			else {
				CoutT("Acceptor error: ", ec, "\n");
			}
			BeginAccept();
		});
	}
};

int main(int argc, char* argv[])
{
	try {
		asio::io_context ios;
		Acceptor acceptor(ios, 12345);
		ios.run();
	}
	catch (std::exception& e) {
		Cout("Exception: ", e.what(), "\n");
	}
	return 0;
}
