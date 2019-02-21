#pragma once
#include "xx_uv_base.h"
#include "xx_buffer.h"
#include <functional>
#include <array>
#include <string>
#include <memory>

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

// 支持 4 字节长度 header 包收发

struct UvTcpPackPeer : UvTcpBasePeer {
	Buffer buf;
	std::function<int(uint8_t const* const& buf, uint32_t const& len)> OnReceivePack;

	inline int Unpack(uint8_t const* const& recvBuf, uint32_t const& recvLen) noexcept override {
		buf.Append(recvBuf, recvLen);
		uint32_t offset = 0;
		while (offset + 4 <= buf.len) {							// ensure header len( 4 bytes )
			auto len = buf[offset + 0] + (buf[offset + 1] << 8) + (buf[offset + 2] << 16) + (buf[offset + 3] << 24);
			if (len <= 0 /* || len > maxLimit */) return -1;	// invalid length
			if (offset + 4 + len > buf.len) break;				// not enough data

			offset += 4;
			if (int r = HandlePack(buf.buf + offset, len)) return r;
			offset += len;
		}
		buf.RemoveFront(offset);
		return 0;
	}

	inline virtual int HandlePack(uint8_t const* const& buf, uint32_t const& len) noexcept {
		return OnReceivePack ? OnReceivePack(buf, len) : 0;
	};

	// reqbuf = uv_write_t_ex space + len space + data
	// len = data's len
	inline int SendReqPack(uint8_t* const& reqbuf, uint32_t const& len) {
		reqbuf[sizeof(uv_write_t_ex) + 0] = uint8_t(len);		// fill package len
		reqbuf[sizeof(uv_write_t_ex) + 1] = uint8_t(len >> 8);
		reqbuf[sizeof(uv_write_t_ex) + 2] = uint8_t(len >> 16);
		reqbuf[sizeof(uv_write_t_ex) + 3] = uint8_t(len >> 24);

		auto req = (uv_write_t_ex*)reqbuf;						// fill req args
		req->buf.base = (char*)(req + 1);
		req->buf.len = decltype(uv_buf_t::len)(len + 4);
		return Send(req);
	}

	// slow. more memcpy than SendPackCore
	inline int SendPack(uint8_t* const& data, uint32_t const& len) {
		assert(data && len);
		auto buf = (uint8_t*)malloc(sizeof(uv_write_t_ex) + 4 + len);
		memcpy(buf + sizeof(uv_write_t_ex) + 4, data, len);
		return SendReqPack(buf, len);
	}
};

struct UvTcpPackListener : UvTcpBaseListener {
	inline virtual std::shared_ptr<UvTcpBasePeer> CreatePeer() noexcept override {
		return OnCreatePeer ? OnCreatePeer() : std::make_shared<UvTcpPackPeer>();
	}
};

struct UvTcpPackClient : UvTcpBaseClient {
	using UvTcpBaseClient::UvTcpBaseClient;
	inline virtual std::shared_ptr<UvTcpBasePeer> CreatePeer() noexcept override {
		return OnCreatePeer ? OnCreatePeer() : std::make_shared<UvTcpPackPeer>();
	}

	inline std::shared_ptr<UvTcpPackPeer> Peer() noexcept {
		return std::static_pointer_cast<UvTcpPackPeer>(peer);
	}
};
