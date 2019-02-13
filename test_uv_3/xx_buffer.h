#pragma once
#include <memory>
#include <cassert>
#include <cstdint>
struct Buffer {
	uint8_t* buf;
	size_t len;
	size_t cap;

	Buffer(size_t const& cap = 0) {
		buf = cap ? (uint8_t*)malloc(cap) : nullptr;
		len = 0;
		this->cap = cap;
	}
	Buffer(Buffer&&) = default;
	Buffer(Buffer const&) = delete;
	Buffer& operator=(Buffer&&) = default;
	Buffer& operator=(Buffer const&) = delete;

	inline void Reserve(size_t const& cap) noexcept {
		if (cap <= this->cap) return;
		buf = (uint8_t*)realloc(buf, cap);
		this->cap = cap;
	}
	template<typename T, typename = std::enable_if_t<sizeof(T) == 1>>
	inline void Append(T const* const& buf, size_t const& len) {
		Reserve(this->len + len);
		memcpy(this->buf + this->len, buf, len);
		this->len += len;
	}
	inline void RemoveFront(size_t const& len) {
		if (!len) return;
		if (len < this->len) {
			memmove(buf, buf + len, this->len - len);
		}
		this->len -= len;
	}
	inline uint8_t operator[](size_t const& idx) noexcept {
		assert(idx < len);
		return buf[idx];
	}
	inline void Clear(bool const& release) noexcept {
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
