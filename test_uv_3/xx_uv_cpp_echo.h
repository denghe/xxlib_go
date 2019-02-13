#pragma once
#include "xx_uv_cpp.h"

// 支持收啥发啥的 peer

struct EchoPeer : UvTcpPeer {
	inline int Unpack(char const* const& buf, size_t const& len) noexcept override {
		Send(buf, len);
		return 0;
	}
};

struct EchoListener : UvTcpListener {
	inline virtual std::shared_ptr<UvTcpPeer> OnCreatePeer() noexcept override {
		return std::make_shared<EchoPeer>();
	}
};
