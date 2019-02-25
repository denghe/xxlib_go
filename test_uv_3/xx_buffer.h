#pragma once
#include "xx_list.h"
#include <memory>
#include <cassert>
namespace xx {

	struct Buffer : List<uint8_t> {
		using BaseType = List<uint8_t>;
		using BaseType::BaseType;

		Buffer(uint8_t const* const& buf, size_t const& len, size_t const& prefix = 0) {
			assert(buf && len);
			this->buf = (uint8_t*)malloc(len + prefix);
			this->len = len + prefix;
			this->cap = cap + prefix;
			memcpy(this->buf + prefix, buf, len);
		}
		Buffer(Buffer&& o) 
			: BaseType(std::move(o)) {
		}

		Buffer& operator=(Buffer&& o) {
			this->BaseType::operator=(std::move(o));
			return *this;
		}

		Buffer(Buffer const&) = delete;
		Buffer& operator=(Buffer const&) = delete;

		inline void RemoveFront(size_t const& len) {
			assert(len <= this->len);
			if (!len) return;
			this->len -= len;
			if (this->len) {
				memmove(buf, buf + len, this->len);
			}
		}
	};
}
