#pragma once
#include "xx_list.h"

namespace PKG {
	struct PkgGenMd5 {
		inline static const std::string value = "0699ba252c4258f684959eeff94f6dd8";
    };

    struct Player;
    using Player_s = std::shared_ptr<Player>;
    using Player_w = std::weak_ptr<Player>;

    struct Monster;
    using Monster_s = std::shared_ptr<Monster>;
    using Monster_w = std::weak_ptr<Monster>;

    struct Scene;
    using Scene_s = std::shared_ptr<Scene>;
    using Scene_w = std::weak_ptr<Scene>;

    // 当需要序列化同时携带玩家数据时, 临时构造这个结构体出来发送以满足 Ref<Player> 数据存放问题
    struct Sync;
    using Sync_s = std::shared_ptr<Sync>;
    using Sync_w = std::weak_ptr<Sync>;

    struct Foo;
    using Foo_s = std::shared_ptr<Foo>;
    using Foo_w = std::weak_ptr<Foo>;

    struct Player : xx::Object {
        int32_t id = 0;
        std::string_s token;
        std::weak_ptr<PKG::Monster> target;

        typedef Player ThisType;
        typedef xx::Object BaseType;
	    Player() = default;
		Player(Player const&) = delete;
		Player& operator=(Player const&) = delete;

        void ToString(std::string& s) const noexcept override;
        void ToStringCore(std::string& s) const noexcept override;

        virtual uint16_t GetTypeId() const noexcept;
        void ToBBuffer(xx::BBuffer& bb) const noexcept override;
        int FromBBuffer(xx::BBuffer& bb) noexcept override;
        int FromBBufferCore(xx::BBuffer& bb) noexcept;

        static std::shared_ptr<Player> MakeShared() noexcept;
        inline static std::shared_ptr<ThisType> defaultInstance;
    };
    struct Monster : xx::Object {

        typedef Monster ThisType;
        typedef xx::Object BaseType;
	    Monster() = default;
		Monster(Monster const&) = delete;
		Monster& operator=(Monster const&) = delete;

        void ToString(std::string& s) const noexcept override;
        void ToStringCore(std::string& s) const noexcept override;

        virtual uint16_t GetTypeId() const noexcept;
        void ToBBuffer(xx::BBuffer& bb) const noexcept override;
        int FromBBuffer(xx::BBuffer& bb) noexcept override;
        int FromBBufferCore(xx::BBuffer& bb) noexcept;

        static std::shared_ptr<Monster> MakeShared() noexcept;
        inline static std::shared_ptr<ThisType> defaultInstance;
    };
    struct Scene : xx::Object {
        xx::List_s<PKG::Monster_s> monsters;
        xx::List_s<std::weak_ptr<PKG::Player>> players;

        typedef Scene ThisType;
        typedef xx::Object BaseType;
	    Scene() = default;
		Scene(Scene const&) = delete;
		Scene& operator=(Scene const&) = delete;

        void ToString(std::string& s) const noexcept override;
        void ToStringCore(std::string& s) const noexcept override;

        virtual uint16_t GetTypeId() const noexcept;
        void ToBBuffer(xx::BBuffer& bb) const noexcept override;
        int FromBBuffer(xx::BBuffer& bb) noexcept override;
        int FromBBufferCore(xx::BBuffer& bb) noexcept;

        static std::shared_ptr<Scene> MakeShared() noexcept;
        inline static std::shared_ptr<ThisType> defaultInstance;
    };
    // 当需要序列化同时携带玩家数据时, 临时构造这个结构体出来发送以满足 Ref<Player> 数据存放问题
    struct Sync : xx::Object {
        xx::List_s<PKG::Player_s> players;
        PKG::Scene_s scene;

        typedef Sync ThisType;
        typedef xx::Object BaseType;
	    Sync() = default;
		Sync(Sync const&) = delete;
		Sync& operator=(Sync const&) = delete;

        void ToString(std::string& s) const noexcept override;
        void ToStringCore(std::string& s) const noexcept override;

        virtual uint16_t GetTypeId() const noexcept;
        void ToBBuffer(xx::BBuffer& bb) const noexcept override;
        int FromBBuffer(xx::BBuffer& bb) noexcept override;
        int FromBBufferCore(xx::BBuffer& bb) noexcept;

        static std::shared_ptr<Sync> MakeShared() noexcept;
        inline static std::shared_ptr<ThisType> defaultInstance;
    };
    struct Foo : xx::Object {
        std::weak_ptr<PKG::Foo> parent;
        xx::List_s<std::weak_ptr<PKG::Foo>> childs;

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

        static std::shared_ptr<Foo> MakeShared() noexcept;
        inline static std::shared_ptr<ThisType> defaultInstance;
    };
}
namespace xx {
    template<> struct TypeId<PKG::Player> { static const uint16_t value = 5; };
    template<> struct TypeId<PKG::Monster> { static const uint16_t value = 6; };
    template<> struct TypeId<PKG::Scene> { static const uint16_t value = 7; };
    template<> struct TypeId<xx::List<PKG::Monster_s>> { static const uint16_t value = 8; };
    template<> struct TypeId<xx::List<std::weak_ptr<PKG::Player>>> { static const uint16_t value = 9; };
    template<> struct TypeId<PKG::Sync> { static const uint16_t value = 10; };
    template<> struct TypeId<xx::List<PKG::Player_s>> { static const uint16_t value = 11; };
    template<> struct TypeId<PKG::Foo> { static const uint16_t value = 3; };
    template<> struct TypeId<xx::List<std::weak_ptr<PKG::Foo>>> { static const uint16_t value = 4; };
}
namespace PKG {
    inline uint16_t Player::GetTypeId() const noexcept {
        return 5;
    }
    inline void Player::ToBBuffer(xx::BBuffer& bb) const noexcept {
        bb.Write(this->id);
        bb.Write(this->target);
    }
    inline int Player::FromBBuffer(xx::BBuffer& bb) noexcept {
        return this->FromBBufferCore(bb);
    }
    inline int Player::FromBBufferCore(xx::BBuffer& bb) noexcept {
        if (int r = bb.Read(this->id)) return r;
        bb.readLengthLimit = 0;
        if (int r = bb.Read(this->token)) return r;
        if (int r = bb.Read(this->target)) return r;
        return 0;
    }
    inline void Player::ToString(std::string& s) const noexcept {
        if (this->toStringFlag)
        {
        	xx::Append(s, "[ \"***** recursived *****\" ]");
        	return;
        }
        else this->SetToStringFlag();

        xx::Append(s, "{ \"pkgTypeName\":\"Player\", \"pkgTypeId\":", GetTypeId());
        ToStringCore(s);
        xx::Append(s, " }");
        
        this->SetToStringFlag(false);
    }
    inline void Player::ToStringCore(std::string& s) const noexcept {
        this->BaseType::ToStringCore(s);
        xx::Append(s, ", \"id\":", this->id);
        if (this->token) xx::Append(s, ", \"token\":\"", this->token, "\"");
        else xx::Append(s, ", \"token\":nil");
        xx::Append(s, ", \"target\":", this->target);
    }
    inline std::shared_ptr<Player> Player::MakeShared() noexcept {
        return std::make_shared<Player>();
    }
    inline uint16_t Monster::GetTypeId() const noexcept {
        return 6;
    }
    inline void Monster::ToBBuffer(xx::BBuffer& bb) const noexcept {
    }
    inline int Monster::FromBBuffer(xx::BBuffer& bb) noexcept {
        return this->FromBBufferCore(bb);
    }
    inline int Monster::FromBBufferCore(xx::BBuffer& bb) noexcept {
        return 0;
    }
    inline void Monster::ToString(std::string& s) const noexcept {
        if (this->toStringFlag)
        {
        	xx::Append(s, "[ \"***** recursived *****\" ]");
        	return;
        }
        else this->SetToStringFlag();

        xx::Append(s, "{ \"pkgTypeName\":\"Monster\", \"pkgTypeId\":", GetTypeId());
        ToStringCore(s);
        xx::Append(s, " }");
        
        this->SetToStringFlag(false);
    }
    inline void Monster::ToStringCore(std::string& s) const noexcept {
        this->BaseType::ToStringCore(s);
    }
    inline std::shared_ptr<Monster> Monster::MakeShared() noexcept {
        return std::make_shared<Monster>();
    }
    inline uint16_t Scene::GetTypeId() const noexcept {
        return 7;
    }
    inline void Scene::ToBBuffer(xx::BBuffer& bb) const noexcept {
        bb.Write(this->monsters);
        bb.Write(this->players);
    }
    inline int Scene::FromBBuffer(xx::BBuffer& bb) noexcept {
        return this->FromBBufferCore(bb);
    }
    inline int Scene::FromBBufferCore(xx::BBuffer& bb) noexcept {
        bb.readLengthLimit = 0;
        if (int r = bb.Read(this->monsters)) return r;
        bb.readLengthLimit = 0;
        if (int r = bb.Read(this->players)) return r;
        return 0;
    }
    inline void Scene::ToString(std::string& s) const noexcept {
        if (this->toStringFlag)
        {
        	xx::Append(s, "[ \"***** recursived *****\" ]");
        	return;
        }
        else this->SetToStringFlag();

        xx::Append(s, "{ \"pkgTypeName\":\"Scene\", \"pkgTypeId\":", GetTypeId());
        ToStringCore(s);
        xx::Append(s, " }");
        
        this->SetToStringFlag(false);
    }
    inline void Scene::ToStringCore(std::string& s) const noexcept {
        this->BaseType::ToStringCore(s);
        xx::Append(s, ", \"monsters\":", this->monsters);
        xx::Append(s, ", \"players\":", this->players);
    }
    inline std::shared_ptr<Scene> Scene::MakeShared() noexcept {
        return std::make_shared<Scene>();
    }
    inline uint16_t Sync::GetTypeId() const noexcept {
        return 10;
    }
    inline void Sync::ToBBuffer(xx::BBuffer& bb) const noexcept {
        bb.Write(this->players);
        bb.Write(this->scene);
    }
    inline int Sync::FromBBuffer(xx::BBuffer& bb) noexcept {
        return this->FromBBufferCore(bb);
    }
    inline int Sync::FromBBufferCore(xx::BBuffer& bb) noexcept {
        bb.readLengthLimit = 0;
        if (int r = bb.Read(this->players)) return r;
        if (int r = bb.Read(this->scene)) return r;
        return 0;
    }
    inline void Sync::ToString(std::string& s) const noexcept {
        if (this->toStringFlag)
        {
        	xx::Append(s, "[ \"***** recursived *****\" ]");
        	return;
        }
        else this->SetToStringFlag();

        xx::Append(s, "{ \"pkgTypeName\":\"Sync\", \"pkgTypeId\":", GetTypeId());
        ToStringCore(s);
        xx::Append(s, " }");
        
        this->SetToStringFlag(false);
    }
    inline void Sync::ToStringCore(std::string& s) const noexcept {
        this->BaseType::ToStringCore(s);
        xx::Append(s, ", \"players\":", this->players);
        xx::Append(s, ", \"scene\":", this->scene);
    }
    inline std::shared_ptr<Sync> Sync::MakeShared() noexcept {
        return std::make_shared<Sync>();
    }
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
    inline std::shared_ptr<Foo> Foo::MakeShared() noexcept {
        return std::make_shared<Foo>();
    }
}
namespace PKG {
	inline void AllTypesRegister() noexcept {
	    xx::BBuffer::Register<PKG::Player>(5);
	    xx::BBuffer::Register<PKG::Monster>(6);
	    xx::BBuffer::Register<PKG::Scene>(7);
	    xx::BBuffer::Register<xx::List<PKG::Monster_s>>(8);
	    xx::BBuffer::Register<xx::List<std::weak_ptr<PKG::Player>>>(9);
	    xx::BBuffer::Register<PKG::Sync>(10);
	    xx::BBuffer::Register<xx::List<PKG::Player_s>>(11);
	    xx::BBuffer::Register<PKG::Foo>(3);
	    xx::BBuffer::Register<xx::List<std::weak_ptr<PKG::Foo>>>(4);
	}
}
