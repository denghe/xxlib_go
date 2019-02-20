#pragma once
#include "xx_uv_base.h"
#include <chrono>
#include <iostream>
#include <thread>

// 支持收啥发啥的 peer

struct EchoPeer : UvTcpBasePeer {
	inline int Unpack(uint8_t const* const& buf, uint32_t const& len) noexcept override {
		return Send(buf, len);
	}
};

struct EchoListener : UvTcpBaseListener {
	inline virtual std::shared_ptr<UvTcpBasePeer> CreatePeer() noexcept override {
		return std::make_shared<EchoPeer>();
	}
};

struct EchoClientPeer : UvTcpBasePeer {
	std::chrono::time_point<std::chrono::system_clock> t = std::chrono::system_clock::now();
	int count = 0;
	inline int Unpack(uint8_t const* const& buf, uint32_t const& len) noexcept override {
		if (++count > 100000) {
			std::cout << double(std::chrono::nanoseconds(std::chrono::system_clock::now() - t).count()) / 1000000000 << std::endl;
			return -1;
		}
		return Send(buf, len);
	}
};

struct EchoClient : UvTcpBaseClient {
	using UvTcpBaseClient::UvTcpBaseClient;
	inline virtual std::shared_ptr<UvTcpBasePeer> CreatePeer() noexcept override {
		return std::make_shared<EchoClientPeer>();
	}
	inline virtual void OnConnect(std::shared_ptr<UvTcpBasePeer> peer) noexcept override {
		peer->Send((uint8_t*)"a", 1);
	}
};

void TestUvEcho() {
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
