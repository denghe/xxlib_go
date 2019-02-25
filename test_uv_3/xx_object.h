#pragma once
#include <stdint.h>
#include <string>
#include <initializer_list>
#include <vector>
#include <memory>
#include <type_traits>

namespace std {
	using string_s = shared_ptr<string>;
	using string_w = weak_ptr<string>;
	template<typename T>
	using vector_s = shared_ptr<vector<T>>;
	template<typename T>
	using vector_w = weak_ptr<vector<T>>;
}

namespace xx {
	struct BBuffer;

	struct Object {
		virtual ~Object() {}

		inline virtual uint16_t GetTypeId() const noexcept { return 0; }
		inline virtual void ToBBuffer(BBuffer& bb) const noexcept {}
		inline virtual int FromBBuffer(BBuffer& bb) noexcept { return 0; }

		inline virtual void ToString(std::string& s) const noexcept {};
		inline virtual void ToStringCore(std::string& s) const noexcept {};

		bool toStringFlag = false;
		inline void SetToStringFlag(bool const& b = true) const noexcept {
			((Object*)this)->toStringFlag = b;
		}
	};

	using Object_s = std::shared_ptr<Object>;

	// TypeId 映射
	template<typename T>
	struct TypeId {
		static const uint16_t value = 0;
	};

	template<typename T>
	constexpr uint16_t TypeId_v = TypeId<T>::value;


	// 序列化 基础适配模板
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

	// 字符串 基础适配模板
	template<typename T, typename ENABLED = void>
	struct SFuncs {
		static inline void WriteTo(std::string& s, T const& in) noexcept {
			assert(false);
		}
	};

	// for easy use
	template<typename T>
	void AppendCore(std::string& s, T const& v) {
		SFuncs<T>::WriteTo(s, v);
	}

	template<typename ...Args>
	void Append(std::string& s, Args const& ... args) {
		std::initializer_list<int> n{ ((AppendCore(s, args)), 0)... };
		(void)(n);
	}

	// 适配 char* \0 结尾 字串( 不是很高效 )
	template<>
	struct SFuncs<char*, void> {
		static inline void WriteTo(std::string& s, char* const& in) noexcept {
			if (in) {
				s.append(in);
			};
		}
	};

	// 适配 char const* \0 结尾 字串( 不是很高效 )
	template<>
	struct SFuncs<char const*, void> {
		static inline void WriteTo(std::string& s, char const* const& in) noexcept {
			if (in) {
				s.append(in);
			};
		}
	};

	// 适配 literal char[len] string
	template<size_t len>
	struct SFuncs<char[len], void> {
		static inline void WriteTo(std::string& s, char const(&in)[len]) noexcept {
			s.append(in);
		}
	};

	// 适配所有数字
	template<typename T>
	struct SFuncs<T, std::enable_if_t<std::is_arithmetic_v<T>>> {
		static inline void WriteTo(std::string& s, T const &in) noexcept {
			if constexpr (std::is_same_v<bool, T>) {
				s.append(in ? "true" : "false");
			}
			else if constexpr (std::is_same_v<char, T>) {
				s.append(in);
			}
			else {
				s.append(std::to_string(in));
			}
		}
	};

	// 适配 enum( 根据原始数据类型调上面的适配 )
	template<typename T>
	struct SFuncs<T, std::enable_if_t<std::is_enum_v<T>>> {
		static inline void WriteTo(std::string& s, T const &in) noexcept {
			s.append((std::underlying_type_t<T>)in);
		}
	};

	// 适配 Object
	template<typename T>
	struct SFuncs<T, std::enable_if_t<std::is_base_of_v<Object, T>>> {
		static inline void WriteTo(std::string& s, T const &in) noexcept {
			in.ToString(s);
		}
	};

	// 适配 std::string
	template<typename T>
	struct SFuncs<T, std::enable_if_t<std::is_base_of_v<std::string, T>>> {
		static inline void WriteTo(std::string& s, T const &in) noexcept {
			s.append(in);
		}
	};

	// 适配 std::shared_ptr<T>
	template<typename T>
	struct SFuncs<std::shared_ptr<T>, std::enable_if_t<std::is_base_of_v<Object, T> || std::is_same_v<std::string, T>>> {
		static inline void WriteTo(std::string& s, std::shared_ptr<T> const& in) noexcept {
			if (in) {
				SFuncs<T>::WriteTo(s, *in);
			}
			else {
				s.append("nil");
			}
		}
	};

	// 适配 std::weak_ptr<T>
	template<typename T>
	struct SFuncs<std::weak_ptr<T>, std::enable_if_t<std::is_base_of_v<Object, T> || std::is_same_v<std::string, T>>> {
		static inline void WriteTo(std::string& s, std::weak_ptr<T> const& in) noexcept {
			if (auto o = in.lock()) {
				SFuncs<T>::WriteTo(s, *o);
			}
			else {
				s.append("nil");
			}
		}
	};

	// 适配 std::vector<T>
	template<typename T>
	struct SFuncs<std::vector<T>, void> {
		static inline void WriteTo(std::string& s, std::vector<T> const& in) noexcept {
			s.append("[ ");
			for (size_t i = 0; i < in.size(); i++)
			{
				Append(s, in[i], ", ");
			}
			if (in.size())
			{
				s.resize(s.size() - 2);
				s.append(" ]");
			}
			else
			{
				s[s.size() - 1] = ']';
			}
		}
	};



	// utils

	inline size_t Calc2n(size_t const& n) noexcept {
		assert(n);
#ifdef _MSC_VER
		unsigned long r = 0;
#if defined(_WIN64) || defined(_M_X64)
		_BitScanReverse64(&r, n);
# else
		_BitScanReverse(&r, n);
# endif
		return (size_t)r;
#else
#if defined(__LP64__) || __WORDSIZE == 64
		return int(63 - __builtin_clzl(n));
# else
		return int(31 - __builtin_clz(n));
# endif
#endif
	}

	inline size_t Round2n(size_t const& n) noexcept {
		auto rtv = size_t(1) << Calc2n(n);
		if (rtv == n) return n;
		else return rtv << 1;
	}




	// std::cout 扩展

	inline std::ostream& operator<<(std::ostream& os, const Object& o) {
		std::string s;
		o.ToString(s);
		os << s;
		return os;
	}

	template<typename T>
	std::ostream& operator<<(std::ostream& os, std::shared_ptr<T> const& o) {
		if (!o) return os << "nil";
		return os << *o;
	}

	template<typename T>
	std::ostream& operator<<(std::ostream& os, std::weak_ptr<T> const& o) {
		if (!o) return os << "nil";
		return os << *o;
	}

}
