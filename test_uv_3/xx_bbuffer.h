#pragma once
#include "xx_buffer.h"
#include <string>
#include <initializer_list>
#include <unordered_map>
#include <array>

struct BBuffer;

// 基础适配模板
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

// 所有可序列化类的基类
struct BObject {
	inline virtual uint16_t GetTypeId() const noexcept { return 0; }
	inline virtual void ToBBuffer(BBuffer& bb) const noexcept {}
	inline virtual int FromBBuffer(BBuffer& bb) noexcept { return 0; }
};

struct BBuffer : Buffer, BObject {
	uint32_t offset = 0;													// 读指针偏移量
	uint32_t offsetRoot = 0;												// offset值写入修正
	uint32_t readLengthLimit = 0;											// 主用于传递给容器类进行长度合法校验

	std::unordered_map<void*, uint32_t> ptrs;
	std::unordered_map<uint32_t, std::shared_ptr<BObject>> idxs;
	std::unordered_map<uint32_t, std::shared_ptr<std::string>> idxs1;


	using Buffer::Buffer;
	BBuffer(BBuffer const&) = delete;
	BBuffer& operator=(BBuffer&&) = default;
	BBuffer& operator=(BBuffer const&) = delete;


	typedef std::shared_ptr<BObject>(*Creator)();
	inline static std::array<Creator, 1 << (sizeof(uint16_t) * 8)> creators;

	inline static void Register(uint16_t const& typeId, Creator c) noexcept {
		creators[typeId] = c;
	}

	inline static std::shared_ptr<BObject> CreateByTypeId(uint16_t typeId) {
		return creators[typeId] ? creators[typeId]() : std::shared_ptr<BObject>();
	}

	template<typename ...TS>
	void Write(TS const& ...vs) noexcept;

	template<typename T, typename ...TS>
	int ReadCore(T& v, TS&...vs) noexcept;

	template<typename T>
	int ReadCore(T& v) noexcept;

	template<typename ...TS>
	int Read(TS&...vs) noexcept;

	template<typename T>
	void WriteRoot(std::shared_ptr<T> const& v) noexcept {
		ptrs.clear();
		offsetRoot = len;
		Write(v);
	}

	template<typename T>
	int ReadRoot(std::shared_ptr<T>& v) noexcept {
		idxs.clear();
		idxs1.clear();
		offsetRoot = offset;
		int r = Read(v);
		idxs.clear();
		idxs1.clear();
		return r;
	}

	template<typename T>
	void WritePtr(std::shared_ptr<T> const& v) noexcept {
		static_assert(std::is_base_of_v<BObject, T> || std::is_same_v<std::string, T>, "not support type??");
		uint16_t typeId = 0;
		if (!v) {
			Write(typeId);
			return;
		}
		if constexpr (std::is_base_of_v<BObject, T>) {
			typeId = v->GetTypeId();
			assert(typeId);					// forget Register TypeId ? 
		}
		else typeId = 1;					// std::string

		Write(typeId);

		auto iter = ptrs.find((void*)&*v);
		uint32_t offs;
		if (iter == ptrs.end()) {
			offs = len - offsetRoot;
			ptrs[(void*)&*v] = offs;
		}
		else {
			offs = iter->second;
		}
		Write(offs);
		if (iter == ptrs.end()) {
			if constexpr (std::is_same_v<std::string, T>) {
				Write(*v);
			}
			else {
				v->ToBBuffer(*this);
			}
		}
	}

	template<typename T>
	int ReadPtr(std::shared_ptr<T>& v) noexcept {
		static_assert(std::is_base_of_v<BObject, T> || std::is_same_v<std::string, T>, "not support type??");
		v.reset();
		uint16_t typeId;
		if (auto r = Read(typeId)) return r;
		if (typeId == 0) return 0;
		if constexpr (std::is_same_v<std::string, T>) {
			if (typeId != 1) return -2;
		}
		else if constexpr (std::is_base_of_v<BObject, T>) {
			if (typeId < 2) return -2;
		}
		if (typeId > 2 && !creators[typeId]) return -1;

		auto offs = offset - offsetRoot;
		uint32_t ptrOffset;
		if (auto r = Read(ptrOffset)) return r;
		if (ptrOffset == offs) {
			if constexpr (std::is_same_v<std::string, T>) {
				v = std::make_shared<std::string>();
				idxs1[ptrOffset] = v;
				if (auto r = Read(*v)) return r;
			}
			else {
				auto o = CreateByTypeId(typeId);
				v = std::dynamic_pointer_cast<T>(o);
				if (!v) return -3;
				idxs[ptrOffset] = o;
				if (auto r = o->FromBBuffer(*this)) return r;
			}
		}
		else {
			if constexpr (std::is_same_v<std::string, T>) {
				auto iter = idxs1.find(ptrOffset);
				if (iter == idxs1.end()) return -4;
				v = iter->second;
			}
			else {
				auto iter = idxs.find(ptrOffset);
				if (iter == idxs.end()) return -5;
				if (iter->second->GetTypeId() != typeId) return -6;
				v = std::dynamic_pointer_cast<T>(iter->second);
				if (!v) return -7;
			}
		}
		return 0;
	}


	template<typename SIn, typename UOut = std::make_unsigned_t<SIn>>
	inline static UOut ZigZagEncode(SIn const& in) noexcept {
		return (in << 1) ^ (in >> (sizeof(SIn) - 1));
	}

	template<typename UIn, typename SOut = std::make_signed_t<UIn>>
	inline static SOut ZigZagDecode(UIn const& in) noexcept {
		return (SOut)(in >> 1) ^ (-(SOut)(in & 1));
	}

	template<typename T>
	inline void WriteVarIntger(T const& v) {
		auto u = std::make_unsigned_t<T>(v);
		if constexpr (std::is_signed_v<T>) {
			u = ZigZagEncode(v);
		}
		Reserve(sizeof(T) + 1);
		while (u >= 1 << 7) {
			buf[len++] = uint8_t(u & 0x7fu | 0x80u);
			u >>= 7;
		};
		buf[len++] = uint8_t(u);
	}

	template<typename T>
	inline int ReadVarInteger(T& v) {
		std::make_unsigned_t<T> u(0);
		for (int shift = 0; shift < sizeof(T) * 8; shift += 7) {
			if (offset == len) return -1;
			auto b = buf[offset++];
			u |= (b & 0x7Fu) << shift;
			if ((b & 0x80) == 0) {
				if constexpr (std::is_signed_v<T>) {
					v = ZigZagDecode(u);
				}
				return 0;
			}
		}
		return -2;
	}


	inline virtual uint16_t GetTypeId() const noexcept override {
		return 2;
	}

	inline virtual void ToBBuffer(BBuffer& bb) const noexcept override {
		assert(this != &bb);
		bb.Write(len);
		bb.Append(buf, len);
	}

	inline virtual int FromBBuffer(BBuffer& bb) noexcept override {
		assert(this != &bb);
		uint32_t dataLen = 0;
		if (auto r = bb.Read(dataLen)) return r;
		if (bb.offset + dataLen > bb.len) return -1;
		Clear();
		Append(bb.buf + bb.offset, dataLen);
		bb.offset += dataLen;
		return 0;
	}


	inline std::string ToString() {
		std::string s;
		for (uint32_t i = 0; i < len; i++) {
			s += std::to_string((int)buf[i]) + " ";
		}
		return s;
	}
};





// 适配 1 字节长度的 数值 或 float( 这些类型直接 memcpy )
template<typename T>
struct BFuncs<T, std::enable_if_t< (std::is_arithmetic_v<T> && sizeof(T) == 1) || (std::is_floating_point_v<T> && sizeof(T) == 4) >> {
	static inline void WriteTo(BBuffer& bb, T const &in) noexcept {
		bb.Reserve(bb.len + sizeof(T));
		memcpy(bb.buf + bb.len, &in, sizeof(T));
		bb.len += sizeof(T);
	}
	static inline int ReadFrom(BBuffer& bb, T &out) noexcept {
		if (bb.offset + sizeof(T) > bb.len) return -1;
		memcpy(&out, bb.buf + bb.offset, sizeof(T));
		bb.offset += sizeof(T);
		return 0;
	}
};

// 适配 2+ 字节整数( 变长读写 )
template<typename T>
struct BFuncs<T, std::enable_if_t<std::is_integral_v<T> && sizeof(T) >= 2>> {
	static inline void WriteTo(BBuffer& bb, T const &in) noexcept {
		bb.WriteVarIntger(in);
	}
	static inline int ReadFrom(BBuffer& bb, T &out) noexcept {
		return bb.ReadVarInteger(out);
	}
};

// 适配 enum( 根据原始数据类型调上面的适配 )
template<typename T>
struct BFuncs<T, std::enable_if_t<std::is_enum_v<T>>> {
	typedef std::underlying_type_t<T> UT;
	static inline void WriteTo(BBuffer& bb, T const &in) noexcept {
		BFuncs<UT>::WriteTo(bb, (UT const&)in);
	}
	static inline int ReadFrom(BBuffer& bb, T &out) noexcept {
		return BFuncs<UT>::ReadFrom(bb, (UT&)out);
	}
};

// 适配 double
template<>
struct BFuncs<double, void> {
	static inline void WriteTo(BBuffer& bb, double const &in) noexcept {
		bb.Reserve(bb.len + sizeof(double) + 1);
		if (in == 0) {
			bb.buf[bb.len++] = 0;
		}
		else if (std::isnan(in)) {
			bb.buf[bb.len++] = 1;
		}
		else if (in == -std::numeric_limits<double>::infinity()) {	// negative infinity
			bb.buf[bb.len++] = 2;
		}
		else if (in == std::numeric_limits<double>::infinity()) {	// positive infinity
			bb.buf[bb.len++] = 3;
		}
		else {
			auto i = (int32_t)in;
			if (in == (double)i) {
				bb.buf[bb.len++] = 4;
				BFuncs<int32_t>::WriteTo(bb, i);
			}
			else {
				bb.buf[bb.len] = 5;
				memcpy(bb.buf + bb.len + 1, &in, sizeof(double));
				bb.len += sizeof(double) + 1;
			}
		}
	}
	static inline int ReadFrom(BBuffer& bb, double &out) noexcept {
		if (bb.offset >= bb.len) return -1;		// 确保还有 1 字节可读
		switch (bb.buf[bb.offset++]) {			// 跳过 1 字节
		case 0:
			out = 0;
			return 0;
		case 1:
			out = std::numeric_limits<double>::quiet_NaN();
			return 0;
		case 2:
			out = -std::numeric_limits<double>::infinity();
			return 0;
		case 3:
			out = std::numeric_limits<double>::infinity();
			return 0;
		case 4: {
			int32_t i = 0;
			if (auto rtv = BFuncs<int32_t>::ReadFrom(bb, i)) return rtv;
			out = i;
			return 0;
		}
		case 5: {
			if (bb.len < bb.offset + sizeof(double)) return -1;
			memcpy(&out, bb.buf + bb.offset, sizeof(double));
			bb.offset += sizeof(double);
			return 0;
		}
		default:
			return -2;							// failed
		}
	}
};

// 适配 std::string ( 写入 32b长度 + 内容 )
template<>
struct BFuncs<std::string, void> {
	static inline void WriteTo(BBuffer& bb, std::string const& in) noexcept {
		bb.Write((uint32_t)in.size());
		bb.Append(in.data(), (uint32_t)in.size());
	}
	static inline int ReadFrom(BBuffer& bb, std::string& out) noexcept {
		uint32_t len = 0;
		if (auto r = bb.Read(len)) return r;
		if (bb.readLengthLimit && bb.readLengthLimit < len) return -1;
		if (bb.offset + len > bb.len) return -2;
		out.assign((char*)bb.buf + bb.offset, len);
		bb.offset += len;
		return 0;
	}
};

// 适配 std::shared_ptr<T> 以简化 Write Read 书写
template<typename T>
struct BFuncs<std::shared_ptr<T>, std::enable_if_t<std::is_base_of_v<BObject, T> || std::is_same_v<std::string, T>>> {
	static inline void WriteTo(BBuffer& bb, std::shared_ptr<T> const& in) noexcept {
		bb.WritePtr(in);
	}
	static inline int ReadFrom(BBuffer& bb, std::shared_ptr<T>& out) noexcept {
		return bb.ReadPtr(out);
	}
};

template<typename ...TS>
void BBuffer::Write(TS const& ...vs) noexcept {
	std::initializer_list<int> n{ (BFuncs<TS>::WriteTo(*this, vs), 0)... };
}

template<typename T, typename ...TS>
int BBuffer::ReadCore(T& v, TS&...vs) noexcept {
	if (auto r = BFuncs<T>::ReadFrom(*this, v)) return r;
	return ReadCore(vs...);
}

template<typename T>
int BBuffer::ReadCore(T& v) noexcept {
	return BFuncs<T>::ReadFrom(*this, v);
}

template<typename ...TS>
int BBuffer::Read(TS&...vs) noexcept {
	return ReadCore(vs...);
}
