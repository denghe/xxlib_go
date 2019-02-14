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
struct BBuffer;
struct BItem;
struct UvLoopEx : UvLoop {
	using UvLoop::UvLoop;

	typedef std::shared_ptr<BItem> (*Creator)();
	inline static std::array<Creator, 1 << (sizeof(uint16_t) * 8)> creators;

	std::unordered_map<void*, uint32_t> ptrs;
	std::unordered_map<uint32_t, std::shared_ptr<BItem>> idxs;
	std::unordered_map<uint32_t, std::shared_ptr<std::string>> idxs1;

	static void Register(uint16_t const& typeId, Creator c) noexcept {
		creators[typeId] = c;
	}

	static std::shared_ptr<BItem> CreateByTypeId(uint16_t typeId) {
		return creators[typeId] ? creators[typeId]() : std::shared_ptr<BItem>();
	}
};









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
		int r = OnRecv(*this);
		recvs.clear();
		return r;
	}
	Buffer buf = Buffer(65535);
	std::vector<std::shared_ptr<Buffer>> recvs;
	std::function<int(PackagePeer&)> OnRecv;
};

struct PackageListener : UvTcpListener {
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

struct PackageClient : UvTcpClient {
	using UvTcpClient::UvTcpClient;
	inline virtual std::shared_ptr<UvTcpPeer> OnCreatePeer() noexcept override {
		return std::make_shared<PackagePeer>();
	}
	inline virtual void OnConnect(int const& serial, std::weak_ptr<UvTcpPeer> peer_) noexcept override {
		if (auto peer = peer_.lock()) {
			peer->Send("a", 1);
		}
	}
};
