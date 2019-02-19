#pragma once
#include "xx_uv.h"
#include "xx_stackless.h"
#include "xx_bbuffer.h"

struct UvTcpPeerEx : UvTcpPeer {
	BBuffer recvBB;
	BBuffer sendBB;
	std::function<int(std::shared_ptr<BObject>)> OnRecv;

	virtual int HandlePack(char const* const& recvBuf, uint32_t const& recvLen) noexcept override {
		recvBB.buf = (uint8_t*)recvBuf;							// replace recvBB's memory
		recvBB.len = recvLen;
		recvBB.cap = recvLen;
		recvBB.offset = 0;
		std::shared_ptr<BObject> o;
		int r = recvBB.ReadRoot(o);
		recvBB.buf = nullptr;									// restore recvBB's memory
		if (r) return r;
		return OnRecv ? OnRecv(std::move(o)) : 0;
	}

	inline int SendPackage(std::shared_ptr<BObject> const& pkg) {
		sendBB.Reserve(sizeof(uv_write_t_ex) + 4);				// skip uv_write_t_ex + header space
		sendBB.len = sizeof(uv_write_t_ex) + 4;
		sendBB.WriteRoot(pkg);									// append package's serialization data

		auto buf = sendBB.buf;									// cut buf memory for send
		auto len = sendBB.len - (uint32_t)sizeof(uv_write_t_ex) - 4;
		sendBB.buf = nullptr;
		sendBB.len = 0;
		sendBB.cap = 0;

		return SendPack((char*)buf, len);
	}
};

struct UvTcpListenerEx : UvTcpListenerBase {
	inline virtual std::shared_ptr<UvTcpPeerBase> OnCreatePeer() noexcept override {
		return std::make_shared<UvTcpPeerEx>();
	}
};

struct UvTcpClientEx : UvTcpClientBase {
	using UvTcpClientBase::UvTcpClientBase;
	inline virtual std::shared_ptr<UvTcpPeerBase> OnCreatePeer() noexcept override {
		return std::make_shared<UvTcpPeerEx>();
	}
};
