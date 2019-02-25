#pragma once
#include "xx_list.h"

namespace PKG {
	struct PkgGenMd5 {
		inline static const std::string value = "5b987c41b882c0484de8619ebb97edb6";
    };

    struct Foo;
    using Foo_s = std::shared_ptr<Foo>;
    using Foo_w = std::weak_ptr<Foo>;

    struct Foo : xx::Object {
        std::shared_ptr<PKG::Foo> parent;
        xx::List_s<std::shared_ptr<PKG::Foo>> childs;

        typedef Foo ThisType;
        typedef xx::Object BaseType;
	    Foo() = default;
		Foo(Foo const&) = delete;
		Foo& operator=(Foo const&) = delete;

        void ToString(std::string& s) const noexcept override;
        void ToStringCore(std::string& s) const noexcept override;

        virtual uint16_t GetTypeId() const noexcept;
        void ToBBuffer(xx::BBuffer& bb) const noexcept override;
        int FromBBuffer(xx::BBuffer& bb) noexcept override;
        int FromBBufferCore(xx::BBuffer& bb) noexcept;

        static std::shared_ptr<Foo> Create() noexcept;
        inline static std::shared_ptr<ThisType> defaultInstance;
    };
}
namespace xx {
    template<> struct TypeId<PKG::Foo> { static const uint16_t value = 3; };
    template<> struct TypeId<xx::List<std::shared_ptr<PKG::Foo>>> { static const uint16_t value = 4; };
}
namespace PKG {
    inline uint16_t Foo::GetTypeId() const noexcept {
        return 3;
    }
    inline void Foo::ToBBuffer(xx::BBuffer& bb) const noexcept {
        bb.Write(this->parent);
        bb.Write(this->childs);
    }
    inline int Foo::FromBBuffer(xx::BBuffer& bb) noexcept {
        return this->FromBBufferCore(bb);
    }
    inline int Foo::FromBBufferCore(xx::BBuffer& bb) noexcept {
        if (int r = bb.Read(this->parent)) return r;
        bb.readLengthLimit = 0;
        if (int r = bb.Read(this->childs)) return r;
        return 0;
    }
    inline void Foo::ToString(std::string& s) const noexcept {
        if (this->toStringFlag)
        {
        	xx::Append(s, "[ \"***** recursived *****\" ]");
        	return;
        }
        else this->SetToStringFlag();

        xx::Append(s, "{ \"pkgTypeName\":\"Foo\", \"pkgTypeId\":", GetTypeId());
        ToStringCore(s);
        xx::Append(s, " }");
        
        this->SetToStringFlag(false);
    }
    inline void Foo::ToStringCore(std::string& s) const noexcept {
        this->BaseType::ToStringCore(s);
        xx::Append(s, ", \"parent\":", this->parent);
        xx::Append(s, ", \"childs\":", this->childs);
    }
    inline std::shared_ptr<Foo> Foo::Create() noexcept {
        return std::make_shared<Foo>();
    }
}
namespace PKG {
	inline void AllTypesRegister() noexcept {
	    xx::BBuffer::Register<PKG::Foo>(3);
	    xx::BBuffer::Register<xx::List<std::shared_ptr<PKG::Foo>>>(4);
	}
}
