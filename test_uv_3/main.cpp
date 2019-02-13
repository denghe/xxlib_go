#include "xx_uv_cpp.h"
#include "xx_uv_cpp_echo.h"
#include "xx_uv_cpp_package.h"

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

int main() {
	UvLoop uvloop;
	uvloop.CreateListener<EchoListener>("0.0.0.0", 12345);
	uvloop.CreateListener<PackageListener>("0.0.0.0", 12346);
	uvloop.Run();
	return 0;
}
