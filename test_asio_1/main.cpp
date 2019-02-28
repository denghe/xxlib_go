//
// async_tcp_echo_server.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2018 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>

using asio::ip::tcp;

class session
	: public std::enable_shared_from_this<session>
{
public:
	session(tcp::socket socket)
		: socket_(std::move(socket))
	{
	}

	void start()
	{
		do_read();
	}

private:
	void do_read()
	{
		auto self(shared_from_this());
		socket_.async_read_some(asio::buffer(data_, max_length),
			[this, self](std::error_code ec, std::size_t length)
		{
			if (!ec)
			{
				do_write(length);
			}
		});
	}

	void do_write(std::size_t length)
	{
		auto self(shared_from_this());
		asio::async_write(socket_, asio::buffer(data_, length),
			[this, self](std::error_code ec, std::size_t /*length*/)
		{
			if (!ec)
			{
				do_read();
			}
		});
	}

	tcp::socket socket_;
	enum { max_length = 1024 };
	char data_[max_length];
};

class server
{
public:
	server(asio::io_context& io_context, short port)
		: acceptor_(io_context, tcp::endpoint(tcp::v4(), port)),
		socket_(io_context)
	{
		do_accept();
	}

private:
	void do_accept()
	{
		acceptor_.async_accept(socket_,
			[this](std::error_code ec)
		{
			if (!ec)
			{
				std::make_shared<session>(std::move(socket_))->start();
			}

			do_accept();
		});
	}

	tcp::acceptor acceptor_;
	tcp::socket socket_;
};
#include <thread>
int main(int argc, char* argv[])
{
	short port = 12345;
	try
	{
		if (argc < 2)
		{
			port = 12345;
		}
		else
		{
			port = std::atoi(argv[1]);
		}

		asio::io_context io_context;

		server s(io_context, port);

		std::thread thread1([&io_context]() { io_context.run(); });
		std::thread thread2([&io_context]() { io_context.run(); });
		std::thread thread3([&io_context]() { io_context.run(); });
		std::thread thread4([&io_context]() { io_context.run(); });
		std::thread thread5([&io_context]() { io_context.run(); });
		std::thread thread6([&io_context]() { io_context.run(); });
		std::thread thread7([&io_context]() { io_context.run(); });
		std::thread thread8([&io_context]() { io_context.run(); });
		thread1.join();
		thread2.join();
		thread3.join();
		thread4.join();
		thread5.join();
		thread6.join();
		thread7.join();
		thread8.join();
		io_context.run();
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}

	return 0;
}
