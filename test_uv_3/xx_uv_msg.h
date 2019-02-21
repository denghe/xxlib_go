#pragma once
#include "xx_uv_pack.h"
#include "xx_stackless.h"
#include "xx_bbuffer.h"
#include <unordered_map>

#ifdef SendMessage
#undef SendMessage
#endif

struct UvTcpMsgPeer : UvTcpPackPeer, std::enable_shared_from_this<UvTcpMsgPeer> {
	BBuffer recvBB;			// for replace buf memory decode message
	BBuffer sendBB;
	int serial = 0;
	using MsgType = std::shared_ptr<BObject>;
	std::unordered_map<int, std::pair<std::function<int(MsgType&& msg)>, std::shared_ptr<UvTimer>>> callbacks;
	std::function<int(MsgType&& msg)> OnReceivePush;
	std::function<int(int const& serial, MsgType&& msg)> OnReceiveRequest;

	virtual int HandlePack(uint8_t const* const& recvBuf, uint32_t const& recvLen) noexcept override {
		recvBB.buf = (uint8_t*)recvBuf;
		recvBB.len = recvLen;
		recvBB.cap = recvLen;
		recvBB.offset = 0;

		MsgType msg;
		int serial = 0;
		int r = recvBB.Read(serial);
		if (!r) {
			r = recvBB.ReadRoot(msg);
		}
		recvBB.buf = nullptr;
		if (r) return r;
		return ReceiveMessage(std::move(msg), serial);
	}

	// serial == 0: push    > 0: response    < 0: request
	inline int SendMessage(MsgType const& msg, int32_t const& serial = 0) {
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

	inline int SendPush(MsgType const& msg) {
		return SendMessage(msg);
	}

	inline int SendResponse(int32_t const& serial, MsgType const& msg) {
		return SendMessage(msg, serial);
	}

	inline int SendRequest(MsgType const& msg, std::function<int(MsgType&& msg)>&& cb, uint64_t const& timeoutMS = 0) {
		if (Disposed()) return -1;
		std::pair<std::function<int(MsgType&& msg)>, std::shared_ptr<UvTimer>> v;
		++serial;
		if (timeoutMS) {
			v.second = container_of(uvTcp->loop, UvLoop, uvLoop)->CreateTimer(timeoutMS, 0, [this, serial = this->serial]() {
				TimeoutCallback(serial);
			});
			if (!v.second) return -1;
		}
		if (int r = SendMessage(msg, -serial)) return r;
		v.first = std::move(cb);
		callbacks[serial] = std::move(v);
		return 0;
	}

	inline void TimeoutCallback(int const& serial) {
		auto iter = callbacks.find(serial);
		if (iter == callbacks.end()) return;
		iter->second.first(nullptr);
		callbacks.erase(iter);
	}

	inline virtual int ReceiveMessage(MsgType&& msg, int const& serial) {
		if (serial == 0) {
			return OnReceivePush ? OnReceivePush(std::move(msg)) : 0;
		}
		else if (serial < 0) {
			return OnReceiveRequest ? OnReceiveRequest(-serial, std::move(msg)) : 0;
		}
		else {
			auto iter = callbacks.find(serial);
			if (iter == callbacks.end()) return 0;
			int r = iter->second.first(std::move(msg));
			callbacks.erase(iter);
			return r;
		}
	}

	inline virtual void Dispose(bool callback) noexcept override {
		for (decltype(auto) kv : callbacks) {
			kv.second.first(nullptr);
		}
		callbacks.clear();
		this->UvTcpPackPeer::Dispose(callback);
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

	inline std::shared_ptr<UvTcpMsgPeer> Peer() noexcept {
		return std::static_pointer_cast<UvTcpMsgPeer>(peer);
	}
};
