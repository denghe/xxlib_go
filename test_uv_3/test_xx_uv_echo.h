#pragma once
#include "xx_uv_base.h"
#include <chrono>
#include <iostream>
#include <thread>

// 支持收啥发啥的 peer

struct EchoPeer : UvTcpPeerBase {
	inline int Unpack(uint8_t const* const& buf, uint32_t const& len) noexcept override {
		return Send(buf, len);
	}
};

struct EchoListener : UvTcpListenerBase {
	inline virtual std::shared_ptr<UvTcpPeerBase> OnCreatePeer() noexcept override {
		return std::make_shared<EchoPeer>();
	}
};

struct EchoClientPeer : UvTcpPeerBase {
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

struct EchoClient : UvTcpClientBase {
	using UvTcpClientBase::UvTcpClientBase;
	inline virtual std::shared_ptr<UvTcpPeerBase> OnCreatePeer() noexcept override {
		return std::make_shared<EchoClientPeer>();
	}
	inline virtual void OnConnect(std::shared_ptr<UvTcpPeerBase> peer) noexcept override {
		peer->Send("a", 1);
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
