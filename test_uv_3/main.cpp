#include "xx_uv_cpp.h"
#include "xx_coroutine.h"
#include "xx_stackless.h"
#include "xx_bbuffer.h"
#include <iostream>
#include <chrono>

struct PackagePeer : UvTcpPeer {
	BBuffer buf;
	BBuffer sendBB;
	std::vector<std::shared_ptr<BObject>> recvs;
	std::function<int()> OnRecv;

	inline int Unpack(char const* const& recvBuf, uint32_t const& recvLen) noexcept override {
		buf.Append(recvBuf, recvLen);
		uint32_t offset = 0;
		while (offset + 4 <= buf.len) {							// ensure header len( 4 bytes )
			auto len = buf[offset + 0] + (buf[offset + 1] << 8) + (buf[offset + 2] << 16) + (buf[offset + 3] << 24);
			if (len <= 0 /* || len > maxLimit */) return -1;	// invalid length
			if (offset + 4 + len > buf.len) break;				// not enough data

			offset += 4;
			std::shared_ptr<BObject> o;
			buf.offset = offset;
			if (int r = buf.ReadRoot(o)) return r;
			recvs.push_back(std::move(o));
			offset += len;
		}
		buf.RemoveFront(offset);

		if (recvs.empty()) return 0;
		return OnRecv ? 0 : OnRecv();
	}

	inline int SendPackage(std::shared_ptr<BObject> const& pkg) {
		sendBB.Reserve(sizeof(uv_write_t_ex) + 4);
		sendBB.len = sizeof(uv_write_t_ex) + 4;					// skip req + header space
		sendBB.WriteRoot(pkg);

		auto buf = sendBB.buf;									// cut buf for send
		auto len = sendBB.len - sizeof(uv_write_t_ex) - 4;
		sendBB.buf = nullptr;
		sendBB.len = 0;
		sendBB.cap = 0;

		buf[sizeof(uv_write_t_ex) + 0] = uint8_t(len);			// fill package len
		buf[sizeof(uv_write_t_ex) + 1] = uint8_t(len >> 8);
		buf[sizeof(uv_write_t_ex) + 2] = uint8_t(len >> 16);
		buf[sizeof(uv_write_t_ex) + 3] = uint8_t(len >> 24);

		auto req = (uv_write_t_ex*)buf;							// fill req args
		req->buf.base = (char*)(req + 1);
		req->buf.len = decltype(uv_buf_t::len)(len + 4);
		return Send(req);
	}
};

struct TcpPeer : UvTcpPeer {
	inline int Unpack(char const* const& buf, uint32_t const& len) noexcept override {
		std::cout << "recv " << buf[0] << std::endl;
		return -1;	// 断开
	}
};
struct TcpClient : UvTcpClient {
	using UvTcpClient::UvTcpClient;
	inline virtual std::shared_ptr<UvTcpPeer> OnCreatePeer() noexcept override {
		return std::make_shared<TcpPeer>();
	}
};

struct UvLoopCor : UvLoop, Stackless {
	std::chrono::time_point<std::chrono::system_clock> corsLastTime;
	std::chrono::nanoseconds corsDurationPool;
	UvLoopCor(double const& framesPerSecond) : UvLoop() {
		if (funcs.size()) throw - 1;
		auto timer = CreateTimer(0, 1);
		if (!timer) throw -2;
		corsLastTime = std::chrono::system_clock::now();
		corsDurationPool = std::chrono::nanoseconds(0);
		timer->OnFire = [this, timer, nanosPerFrame = std::chrono::nanoseconds(int64_t(1.0 / framesPerSecond * 1000000000)) ] {
			auto currTime = std::chrono::system_clock::now();
			corsDurationPool += currTime - corsLastTime;
			corsLastTime = currTime;
			while (corsDurationPool > nanosPerFrame) {
				if (!RunOnce()) {
					timer->Dispose();
					return;
				};
				corsDurationPool -= nanosPerFrame;
			}
		};
	}
	inline virtual void Stop() noexcept override {
		funcs.clear();
		this->UvLoop::Stop();
	}
};

int main() {
	UvLoopCor loop(61);
	struct Ctx1 {
		std::shared_ptr<TcpClient> client;
		std::shared_ptr<TcpPeer> peer;
		std::chrono::system_clock::time_point t;
	};
	loop.Add([&, zs = std::make_shared<Ctx1>()](int const& lineNumber) {
		switch (lineNumber) {
		case 0:
			zs->client = loop.CreateClient<TcpClient>();
		LabConnect:
			std::cout << "connecting...\n";
			zs->client->Connect("127.0.0.1", 12345);
			zs->t = std::chrono::system_clock::now() + std::chrono::seconds(5);
			while (!zs->client->peer) {
				return 1; case 1:;
				if (std::chrono::system_clock::now() > zs->t) {		// timeout check
					std::cout << "timeout. retry\n";
					goto LabConnect;
				}
			}
			std::cout << "connected.\n";
			zs->peer = std::move(std::static_pointer_cast<TcpPeer>(zs->client->peer));
			zs->peer->Send("a", 1);

			while (!zs->peer->Disposed()) {		// check if disconnected, reconnect
				return 2; case 2:;
			}
			goto LabConnect;
		}
		return (int)0xFFFFFFFF;
	});
	loop.Run();
	std::cout << "end.\n";
	std::cin.get();
	return 0;
}
























//#include "xx_uv_cpp.h"
//#include "xx_uv_cpp_echo.h"
//#include "xx_uv_cpp_package.h"
//
//int main() {
//	TestEcho();
//	return 0;
//}
//
////struct Pos {
////	double x = 0, y = 0;
////};
////template<>
////struct BFuncs<Pos, void> {
////	static inline void WriteTo(BBuffer& bb, Pos const& in) noexcept {
////		bb.Write(in.x, in.y);
////	}
////	static inline int ReadFrom(BBuffer& bb, Pos& out) noexcept {
////		return bb.Read(out.x, out.y);
////	}
////};
//
//#include "xx_bbuffer.h"
//#include <iostream>
//
//struct Node : BObject {
//	int indexAtContainer;
//	std::weak_ptr<Node> parent;
//	std::vector<std::weak_ptr<Node>> childs;
//#pragma region
//	inline virtual uint16_t GetTypeId() const noexcept override {
//		return 3;
//	}
//	inline virtual void ToBBuffer(BBuffer& bb) const noexcept override {
//		bb.Write(this->indexAtContainer, this->parent, this->childs);
//	}
//	inline virtual int FromBBuffer(BBuffer& bb) noexcept override {
//		return bb.Read(this->indexAtContainer, this->parent, this->childs);
//	}
//#pragma endregion
//};
//
//struct Container : BObject {
//	std::vector<std::shared_ptr<Node>> nodes;
//	std::weak_ptr<Node> node;
//#pragma region
//	inline virtual uint16_t GetTypeId() const noexcept override {
//		return 4;
//	}
//	inline virtual void ToBBuffer(BBuffer& bb) const noexcept override {
//		bb.Write(this->nodes, this->node);
//	}
//	inline virtual int FromBBuffer(BBuffer& bb) noexcept override {
//		return bb.Read(this->nodes, this->node);
//	}
//#pragma endregion
//};
//
//int main() {
//	BBuffer::Register<Node>(3);
//	BBuffer::Register<Container>(4);
//
//	auto c = std::make_shared<Container>();
//	auto n = std::make_shared<Node>();
//	n->indexAtContainer = c->nodes.size();
//	c->nodes.push_back(std::move(n));
//	n = std::make_shared<Node>();
//	n->indexAtContainer = c->nodes.size();
//	c->nodes.push_back(std::move(n));
//	c->node = c->nodes[0];
//	c->node.lock()->parent = c->node;
//	c->node.lock()->childs.push_back(c->node);
//	c->node.lock()->childs.push_back(c->nodes[1]);
//
//	BBuffer bb;
//	bb.WriteRoot(c);
//	std::cout << bb.ToString() << std::endl;
//
//	std::shared_ptr<Container> tmp;
//	int r = bb.ReadRoot(tmp);
//	assert(!r);
//	auto c2 = std::dynamic_pointer_cast<Container>(tmp);
//	assert(c2);
//	std::cout << c2->nodes.size()
//		<< " " << (c2->node.lock() == c2->nodes[0])
//		<< " " << (c2->node.lock() == c2->node.lock()->parent.lock())
//		<< " " << c2->node.lock()->childs.size()
//		<< " " << (c2->node.lock()->childs[0].lock() == c2->node.lock())
//		<< " " << (c2->nodes[1] == c2->node.lock()->childs[1].lock())
//		<< std::endl;
//
//	return 0;
//}
