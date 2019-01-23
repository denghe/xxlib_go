//
// blocking_tcp_echo_client.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2018 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <chrono>
#include "asio.hpp"

using asio::ip::tcp;

int main(int argc, char* argv[])
{
	try
	{
		asio::io_context io_context;

		tcp::socket s(io_context);
		tcp::resolver resolver(io_context);
		asio::connect(s, resolver.resolve("127.0.0.1", "12345"));

		char request[128];

		auto t = std::chrono::system_clock::now();
		char reply[128];
		size_t count = 0;
		decltype(auto) writeBuf = asio::buffer(request, 1);
		decltype(auto) readBuf = asio::buffer(reply, 1);
		while (true) {
			asio::write(s, writeBuf);
			auto n = asio::read(s, readBuf);
			if (n == 1) {
				if (++count == 100000) {
					std::cout << double(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now() - t).count()) / 1000000000 << std::endl;
					return 0;
				}
			}
		}
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}

	return 0;
}
