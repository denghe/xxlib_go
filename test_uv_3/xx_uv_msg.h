#pragma once
#include "xx_uv_pack.h"
#include "xx_stackless.h"
#include "xx_bbuffer.h"
#include <unordered_map>

#ifdef SendMessage
#undef SendMessage
#endif

struct UvTcpMsgPeer : UvTcpPackPeer {
	BBuffer recvBB;												// replace buf memory for decode message
	BBuffer sendBB;
	int serial = 0;
	std::unordered_map<int, std::function<int(std::shared_ptr<BObject>&& msg)>> callbacks;
	std::function<int(std::shared_ptr<BObject>&& msg)> OnReceivePush;
	std::function<int(int const& serial, std::shared_ptr<BObject>&& msg)> OnReceiveRequest;

	virtual int HandlePack(uint8_t const* const& recvBuf, uint32_t const& recvLen) noexcept override {
		recvBB.buf = (uint8_t*)recvBuf;
		recvBB.len = recvLen;
		recvBB.cap = recvLen;
		recvBB.offset = 0;

		std::shared_ptr<BObject> o;
		int serial = 0;
		int r = recvBB.Read(serial);
		if (!r) {
			r = recvBB.ReadRoot(o);
		}
		recvBB.buf = nullptr;
		if (r) return r;
		return ReceiveMessage(std::move(o), serial);
	}

	// serial == 0: push    > 0: response    < 0: request
	inline int SendMessage(std::shared_ptr<BObject> const& msg, int32_t const& serial = 0) {
		if (Disposed()) return -1;
		sendBB.Reserve(sizeof(uv_write_t_ex) + 4);				// skip uv_write_t_ex + header space
		sendBB.len = sizeof(uv_write_t_ex) + 4;
		sendBB.Write(serial);									// serial
		sendBB.WriteRoot(msg);									// data

		auto buf = sendBB.buf;									// cut buf memory for send
		auto len = sendBB.len - (uint32_t)sizeof(uv_write_t_ex) - 4;
		sendBB.buf = nullptr;
		sendBB.len = 0;
		sendBB.cap = 0;

		return SendReqPack(buf, len);
	}

	inline int SendPush(std::shared_ptr<BObject> const& msg) {
		return SendMessage(msg);
	}

	inline int SendResponse(int32_t const& serial, std::shared_ptr<BObject> const& msg) {
		return SendMessage(msg, serial);
	}

	inline int SendRequest(std::shared_ptr<BObject> const& msg, std::function<int(std::shared_ptr<BObject>&& msg)>&& cb) {
		if (Disposed()) return -1;
		callbacks[++serial] = std::move(cb);
		SendMessage(msg, -serial);
		return serial;
	}

	inline int RemoveCallback(int const& serial) {
		callbacks.erase(serial);
	}

	inline virtual int ReceiveMessage(std::shared_ptr<BObject>&& msg, int const& serial) {
		if (serial == 0) {
			return OnReceivePush ? OnReceivePush(std::move(msg)) : 0;
		}
		else if (serial < 0) {
			return OnReceiveRequest ? OnReceiveRequest(-serial, std::move(msg)) : 0;
		}
		else {
			auto iter = callbacks.find(serial);
			if (iter == callbacks.end()) return 0;
			auto cb = std::move(iter->second);
			callbacks.erase(iter);
			return cb(std::move(msg));
		}
	}
};

struct UvTcpMsgListener : UvTcpBaseListener {
	inline virtual std::shared_ptr<UvTcpBasePeer> CreatePeer() noexcept override {
		return OnCreatePeer ? OnCreatePeer() : std::make_shared<UvTcpMsgPeer>();
	}
};

struct UvTcpMsgClient : UvTcpBaseClient {
	using UvTcpBaseClient::UvTcpBaseClient;
	inline virtual std::shared_ptr<UvTcpBasePeer> CreatePeer() noexcept override {
		return OnCreatePeer ? OnCreatePeer() : std::make_shared<UvTcpMsgPeer>();
	}
};
