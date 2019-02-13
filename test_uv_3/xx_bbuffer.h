#pragma once
#include "xx_buffer.h"
#include <string>
#include <cstdint>
#include <unordered_map>

struct BBuffer;

// 基础适配模板
template<typename T, typename ENABLED = void>
struct BFunc {
	static void WriteTo(BBuffer& bb, T const& in) noexcept {
		assert(false);
	}
	static int ReadFrom(BBuffer& bb, T& out) noexcept {
		assert(false);
		return 0;
	}
};

// 所有可序列化类的基类
struct BItem {
	virtual uint16_t GetTypeId() const noexcept { return 0; }
	virtual void ToBBuffer(BBuffer& bb) const noexcept {}
	virtual int FromBBuffer(BBuffer& bb) noexcept { return 0; }
};

struct BBuffer : Buffer, BItem {
	using Buffer::Buffer;
	size_t offset = 0;													// 读指针偏移量
	size_t offsetRoot = 0;												// offset值写入修正
	size_t readLengthLimit = 0;											// 主用于传递给容器类进行长度合法校验
	std::unordered_map<void*, size_t> ptrs;
	std::unordered_map<size_t, std::pair<void*, uint16_t>> idxs;

	template<typename T>
	void Write(T const& v) noexcept;

	template<typename T>
	int Read(T& v) noexcept;

	inline void BeginWrite() noexcept {
		ptrs.clear();
		offsetRoot = len;
	}
	void BeginRead() noexcept {
		idxs.clear();
		offsetRoot = offset;
	}

	template<typename T>
	void WriteRoot(std::shared_ptr<T> const& v) noexcept {
		BeginWrite();
		Write(v);
	}

	template<typename T>
	int ReadRoot(std::shared_ptr<T>& v) noexcept {
		BeginRead();
		return Read(v);
	}

	template<typename T>
	void WritePtr(std::shared_ptr<T> const& v) noexcept {
		static_assert(std::is_base_of_v<BItem, T> || std::is_same_v<std::string, T>, "not support type??");
		uint16_t typeId = 0;
		if (!v) {
			Write(typeId);
			return;
		}
		if constexpr (std::is_base_of_v<BItem, T>) {
			typeId = v->GetTypeId();
			assert(typeId);					// forget Register TypeId ? 
		}
		else typeId = 1;					// std::string

		Write(typeId);

		auto notFound = ptrs.find((void*)&*v) == ptrs.end();
		size_t offs;
		if (notFound) {
			offs = len - offsetRoot;
			ptrs[(void*)&*v] = offs;
		}
		Write(offs);
		if (notFound) {
			v->ToBBuffer(*this);
		}
	}

	template<typename T>
	int ReadPtr(std::shared_ptr<T>& v) noexcept {
		static_assert(std::is_base_of_v<BItem, T> || std::is_same_v<std::string, T>, "not support type??");
		uint16_t tid;
		if (auto rtv = Read(tid)) return rtv;

		if (tid == 0) {
			v.reset();
			return 0;
		}

		//// simple validate tid( bad data or forget register typeid ? )
		//if (!MemPool::creators[tid]) return -5;

		//// get offset
		//size_t ptr_offset = 0, bb_offset_bak = offset - offsetRoot;
		//if (auto rtv = Read(ptr_offset)) return rtv;

		//// fill or ref
		//if (ptr_offset == bb_offset_bak)
		//{
		//	// ensure inherit
		//	if (!mempool->IsBaseOf(TypeId<T>::value, tid)) return -2;

		//	// try get creator func
		//	auto& f = MemPool::creators[tid];

		//	// try create & read from bb
		//	v = (T*)f(mempool, this, ptr_offset);
		//	if (v == nullptr) return -3;
		//}
		//else
		//{
		//	// try get ptr from dict
		//	std::pair<void*, uint16_t> val;
		//	if (!mempool->idxStore->TryGetValue(ptr_offset, val)) return -4;

		//	// inherit validate
		//	if (!mempool->IsBaseOf(TypeId<T>::value, val.second)) return -2;

		//	// set val
		//	v = (T*)val.first;
		//}
		return 0;
	}
};


// 先简单实现方便测试. 所有数字类型和枚举类型, 统统 memcpy
template<typename T>
struct BFunc<T, std::enable_if_t<std::is_arithmetic_v<T> || std::is_enum_v<T>>> {
	static inline void WriteTo(BBuffer& bb, T const& in) noexcept {
		bb.Append((char*)&in, sizeof(T));
	}
	static inline int ReadFrom(BBuffer& bb, T& out) noexcept {
		if (bb.offset + sizeof(T) > bb.len) return -1;
		memcpy(&out, bb.buf + bb.offset, sizeof(T));
		bb.offset += sizeof(T);
		return 0;
	}
};

// 适配 std::string ( 写入 32b长度 + 内容 )
template<>
struct BFunc<std::string, void> {
	static inline void WriteTo(BBuffer& bb, std::string const& in) noexcept {
		bb.Write(uint32_t(in.size()));
		bb.Append(in.data(), in.size());
	}
	static inline int ReadFrom(BBuffer& bb, std::string& out) noexcept {
		uint32_t len = 0;
		if (auto r = bb.Read(len)) return r;
		if (bb.offset + len > bb.len) return -1;
		out.assign((char*)bb.buf + bb.offset, len);
		bb.offset += len;
		return 0;
	}
};

// 适配 std::shared_ptr<T> 以简化 Write Read 书写
template<typename T>
struct BFunc<std::shared_ptr<T>, std::enable_if_t<std::is_base_of_v<BItem, T> || std::is_same_v<std::string, T>>> {
	static inline void WriteTo(BBuffer& bb, std::shared_ptr<T> const& in) noexcept {
		bb.WritePtr(in);
	}
	static inline int ReadFrom(BBuffer& bb, std::shared_ptr<T>& out) noexcept {
		return bb.ReadPtr(out);
	}
};

template<typename T>
void BBuffer::Write(T const& v) noexcept {
	BFunc<T>::WriteTo(*this, v);
}

template<typename T>
int BBuffer::Read(T& v) noexcept {
	return BFunc<T>::ReadFrom(*this, v);
}
