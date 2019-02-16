#pragma once
#include "xx_uv_cpp.h"
#include <chrono>
#include <iostream>
#include <thread>

// 支持收啥发啥的 peer

struct EchoPeer : UvTcpPeer {
	inline int Unpack(char const* const& buf, uint32_t const& len) noexcept override {
		return Send(buf, len);
	}
};

struct EchoListener : UvTcpListener {
	inline virtual std::shared_ptr<UvTcpPeer> OnCreatePeer() noexcept override {
		return std::make_shared<EchoPeer>();
	}
};

struct EchoClientPeer : UvTcpPeer {
	std::chrono::time_point<std::chrono::system_clock> t = std::chrono::system_clock::now();
	int count = 0;
	inline int Unpack(char const* const& buf, uint32_t const& len) noexcept override {
		if (++count > 100000) {
			std::cout << double(std::chrono::nanoseconds(std::chrono::system_clock::now() - t).count()) / 1000000000 << std::endl;
			return -1;
		}
		return Send(buf, len);
	}
};

struct EchoClient : UvTcpClient {
	using UvTcpClient::UvTcpClient;
	inline virtual std::shared_ptr<UvTcpPeer> OnCreatePeer() noexcept override {
		return std::make_shared<EchoClientPeer>();
	}
	inline virtual void OnConnect(int const& serial, std::shared_ptr<UvTcpPeer> peer) noexcept override {
		if (peer) {
			peer->Send("a", 1);
		}
	}
};

void TestEcho() {
	std::thread t1([] {
		UvLoop uvloop;
		uvloop.CreateListener<EchoListener>("0.0.0.0", 12345);
		uvloop.Run();
		std::cout << "server end.";
	});
	std::thread t2([] {
		UvLoop uvloop;
		auto client = uvloop.CreateClient<EchoClient>();
		client->Connect("127.0.0.1", 12345);
		uvloop.Run();
		std::cout << "client end.";
	});
	t1.join();
	t2.join();
}
