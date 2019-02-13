﻿#pragma once
#include "xx_uv_cpp.h"
#include "xx_buffer.h"
#include <functional>
#include <deque>

// 支持 4 字节长度包头的 peer

struct PackagePeer : UvTcpPeer {
	inline int Unpack(char const* const& recvBuf, size_t const& recvLen) noexcept override {
		buf.Append(recvBuf, recvLen);
		size_t offset = 0;
		while (offset + 4 <= buf.len) {							// ensure header len( 4 bytes )
			auto len = buf[offset + 0] + (buf[offset + 1] << 8) + (buf[offset + 2] << 16) + (buf[offset + 3] << 24);
			if (len <= 0 /* || len > maxLimit */) return -1;	// invalid length
			if (offset + 4 + len > buf.len) break;				// not enough data

			offset += 4;
			auto recv = std::make_shared<Buffer>(len);
			recv->Append(buf.buf + offset, len);
			recvs.push_back(std::move(recv));
			offset += len;
		}
		buf.RemoveFront(offset);

		if (recvs.empty()) return 0;
		int r = OnRecv(*this);
		recvs.clear();
		return r;
	}
	Buffer buf = Buffer(65535);
	std::vector<std::shared_ptr<Buffer>> recvs;
	std::function<int(PackagePeer&)> OnRecv;
};

struct PackageListenerSample : UvTcpListener {
	inline virtual std::shared_ptr<UvTcpPeer> OnCreatePeer() noexcept override {
		return std::make_shared<PackagePeer>();
	}
	inline virtual void OnAccept(std::weak_ptr<UvTcpPeer> peer) noexcept override {
		auto pp = std::static_pointer_cast<PackagePeer>(peer.lock());
		pp->OnRecv = [](PackagePeer& pp) {
			for (decltype(auto) buf : pp.recvs) {
				// blah blah blah
			}
			return 0;
		};
	};
};