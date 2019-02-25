#pragma once
#include <stdint.h>
#include <string>

namespace xx {
	struct BBuffer;
	
	struct Object {
		inline virtual uint16_t GetTypeId() const noexcept { return 0; }
		inline virtual void ToBBuffer(BBuffer& bb) const noexcept {}
		inline virtual int FromBBuffer(BBuffer& bb) noexcept { return 0; }

		inline virtual void ToString(std::string& s) const noexcept {};
		inline virtual void ToStringCore(std::string& s) const noexcept {};
	};

	// ĞòÁĞ»¯ »ù´¡ÊÊÅäÄ£°å
	template<typename T, typename ENABLED = void>
	struct BFuncs {
		static inline void WriteTo(BBuffer& bb, T const& in) noexcept {
			assert(false);
		}
		static inline int ReadFrom(BBuffer& bb, T& out) noexcept {
			assert(false);
			return 0;
		}
	};

	// ×Ö·û´® »ù´¡ÊÊÅäÄ£°å
	template<typename T, typename ENABLED = void>
	struct SFuncs {
		static inline void WriteTo(std::string& s, T const& in) noexcept {
			assert(false);
		}
	};

}
