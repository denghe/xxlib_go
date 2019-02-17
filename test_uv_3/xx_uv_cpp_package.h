#pragma once
#include "xx_uv_cpp.h"
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

// 支持 4 字节长度包头


struct PackagePeer : UvTcpPeer {
	inline int Unpack(char const* const& recvBuf, uint32_t const& recvLen) noexcept override {
		buf.Append(recvBuf, recvLen);
		uint32_t offset = 0;
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
		int r = OnRecv();
		recvs.clear();
		return r;
	}
	Buffer buf;
	std::vector<std::shared_ptr<Buffer>> recvs;
	std::function<int()> OnRecv;
};

struct PackageListener : UvTcpListener {
	inline virtual std::shared_ptr<UvTcpPeer> OnCreatePeer() noexcept override {
		return std::make_shared<PackagePeer>();
	}
	inline virtual void OnAccept(std::shared_ptr<UvTcpPeer> peer) noexcept override {
		auto pp = std::static_pointer_cast<PackagePeer>(peer);
		pp->OnRecv = [pp] {
			for (decltype(auto) buf : pp->recvs) {
				// blah blah blah
			}
			return 0;
		};
	};
};

struct PackageClient : UvTcpClient {
	using UvTcpClient::UvTcpClient;
	inline virtual std::shared_ptr<UvTcpPeer> OnCreatePeer() noexcept override {
		return std::make_shared<PackagePeer>();
	}
	inline virtual void OnConnect(int const& serial, std::shared_ptr<UvTcpPeer> peer) noexcept override {
		if (peer) {
			peer->Send("a", 1);
		}
	}
};
