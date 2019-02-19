#pragma once
#include "xx_uv_base.h"
#include "xx_stackless.h"
#include <chrono>

struct UvLoopStackless : UvLoop, Stackless {
	std::chrono::time_point<std::chrono::system_clock> corsLastTime;
	std::chrono::nanoseconds corsDurationPool;
	UvLoopStackless(double const& framesPerSecond) : UvLoop() {
		if (funcs.size()) throw - 1;
		auto timer = CreateTimer<>(0, 1);
		if (!timer) throw - 2;
		corsLastTime = std::chrono::system_clock::now();
		corsDurationPool = std::chrono::nanoseconds(0);
		timer->OnFire = [this, timer, nanosPerFrame = std::chrono::nanoseconds(int64_t(1.0 / framesPerSecond * 1000000000))]{
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
