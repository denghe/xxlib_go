#pragma once
#include "xx_object.h"
#include <memory>
#include <cassert>
namespace xx {

	struct Buffer : Object {
		uint8_t* buf;
		uint32_t len;
		uint32_t cap;

		Buffer(uint32_t const& cap = 0) {
			buf = cap ? (uint8_t*)malloc(cap) : nullptr;
			len = 0;
			this->cap = cap;
		}
		Buffer(uint8_t const* const& buf, uint32_t const& len, uint32_t const& prefix = 0) {
			assert(buf && len);
			this->buf = (uint8_t*)malloc(len + prefix);
			this->len = len + prefix;
			this->cap = cap + prefix;
			memcpy(this->buf + prefix, buf, len);
		}
		Buffer(Buffer&& o) : buf(o.buf), len(o.len), cap(o.cap) {
			o.buf = nullptr;
			o.len = 0;
			o.cap = 0;
		}
		Buffer& operator=(Buffer&& o) {
			std::swap(buf, o.buf);
			std::swap(len, o.len);
			std::swap(cap, o.cap);
			return *this;
		}
		Buffer(Buffer const&) = delete;
		Buffer& operator=(Buffer const&) = delete;

		inline void Reserve(uint32_t const& cap) noexcept {
			if (cap <= this->cap) return;
			if (!this->cap) {
				this->cap = 32700;
			}
			while (this->cap < cap) {
				this->cap *= 2;
			}
			buf = (uint8_t*)realloc(buf, this->cap);
		}

		inline void Append(uint8_t const* const& buf, uint32_t const& len) {
			Reserve(this->len + len);
			memcpy(this->buf + this->len, buf, len);
			this->len += len;
		}

		inline void RemoveFront(uint32_t const& len) {
			assert(len <= this->len);
			if (!len) return;
			this->len -= len;
			if (this->len) {
				memmove(buf, buf + len, this->len);
			}
		}

		inline uint8_t operator[](uint32_t const& idx) const noexcept {
			assert(idx < len);
			return buf[idx];
		}
		inline uint8_t& operator[](uint32_t const& idx) noexcept {
			assert(idx < len);
			return buf[idx];
		}

		inline void Clear(bool const& release = false) noexcept {
			if (!buf) return;
			len = 0;
			if (release) {
				free(buf);
				buf = nullptr;
				cap = 0;
			}
		}

		~Buffer() {
			Clear(true);
		}
	};

}
