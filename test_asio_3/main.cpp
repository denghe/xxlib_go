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

struct Peer : std::enable_shared_from_this<Peer> {
	asio::ip::tcp::socket socket;
	std::array<char, 512> buf;
	Peer(asio::ip::tcp::socket&& socket)
		: socket(std::move(socket))
	{
	}
	void BeginRead() {
		socket.async_read_some(asio::buffer(buf), [this, self = shared_from_this()](asio::error_code const& ec, std::size_t const& length) {
			if (!ec) {
				BeginWrite(length);						// echo
			}
			else {
				CoutT("Peer read error: ", ec, "\n");
			}
		});
	}
	void BeginWrite(std::size_t const& length) {
		asio::async_write(socket, asio::buffer(buf, length), [this, self = shared_from_this()](asio::error_code const& ec, std::size_t const& length) {
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
				socket.set_option(asio::ip::tcp::no_delay(true));
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
	CoutT("Begin\n");
	try {
		asio::io_context ios;
		Acceptor acceptor(ios, 12345);
		ios.run();
	}
	catch (std::exception& e) {
		CoutT("Exception: ", e.what(), "\n");
	}
	return 0;
}
