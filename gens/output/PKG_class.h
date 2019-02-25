#pragma once
#include "xx_bbuffer.h"

namespace PKG
{
	struct PkgGenMd5
	{
		inline static const std::string value = "177857543efefea3f77cb3d1a62fefcc";
    };

namespace Generic
{
    class Success;
    using Success_s = std::shared_ptr<Success>;
    using Success_w = std::weak_ptr<Success>;

    class Error;
    using Error_s = std::shared_ptr<Error>;
    using Error_w = std::weak_ptr<Error>;

    class ServerInfo;
    using ServerInfo_s = std::shared_ptr<ServerInfo>;
    using ServerInfo_w = std::weak_ptr<ServerInfo>;

    class UserInfo;
    using UserInfo_s = std::shared_ptr<UserInfo>;
    using UserInfo_w = std::weak_ptr<UserInfo>;

}
namespace Login_DB
{
    // 校验. 成功返回 DB_Login.Auth_Success 内含 userId. 失败返回 Generic.Error
    class Auth;
    using Auth_s = std::shared_ptr<Auth>;
    using Auth_w = std::weak_ptr<Auth>;

}
namespace DB_Login
{
    // Login_DB.Auth 的成功返回值
    class Auth_Success;
    using Auth_Success_s = std::shared_ptr<Auth_Success>;
    using Auth_Success_w = std::weak_ptr<Auth_Success>;

}
namespace Client_Login
{
    // 失败返回 Generic.Error. 成功返回 Login_Client.EnterLobby 或 EnterGame1
    class Auth;
    using Auth_s = std::shared_ptr<Auth>;
    using Auth_w = std::weak_ptr<Auth>;

}
namespace Login_Client
{
    // 校验成功
    class AuthSuccess;
    using AuthSuccess_s = std::shared_ptr<AuthSuccess>;
    using AuthSuccess_w = std::weak_ptr<AuthSuccess>;

}
namespace Generic
{
    class Success : public xx::Object
    {
    public:

        typedef Success ThisType;
        typedef xx::Object BaseType;
	    Success() = default;
		Success(Success const&) = delete;
		Success& operator=(Success const&) = delete;
        virtual uint16_t GetTypeId() const noexcept;
        void ToString(std::string& s) const noexcept override;
        void ToStringCore(std::string& s) const noexcept override;
        void ToBBuffer(xx::BBuffer& bb) const noexcept override;
        int FromBBuffer(xx::BBuffer& bb) noexcept override;
        int FromBBufferCore(xx::BBuffer& bb) noexcept;
        inline static std::shared_ptr<ThisType> defaultInstance;
    };
    class Error : public xx::Object
    {
    public:
        int32_t number = 0;
        xx::String_s text;

        typedef Error ThisType;
        typedef xx::Object BaseType;
	    Error() = default;
		Error(Error const&) = delete;
		Error& operator=(Error const&) = delete;
        virtual uint16_t GetTypeId() const noexcept;
        void ToString(std::string& s) const noexcept override;
        void ToStringCore(std::string& s) const noexcept override;
        void ToBBuffer(xx::BBuffer& bb) const noexcept override;
        int FromBBuffer(xx::BBuffer& bb) noexcept override;
        int FromBBufferCore(xx::BBuffer& bb) noexcept;
        inline static std::shared_ptr<ThisType> defaultInstance;
    };
    class ServerInfo : public xx::Object
    {
    public:
        xx::String_s name;

        typedef ServerInfo ThisType;
        typedef xx::Object BaseType;
	    ServerInfo() = default;
		ServerInfo(ServerInfo const&) = delete;
		ServerInfo& operator=(ServerInfo const&) = delete;
        virtual uint16_t GetTypeId() const noexcept;
        void ToString(std::string& s) const noexcept override;
        void ToStringCore(std::string& s) const noexcept override;
        void ToBBuffer(xx::BBuffer& bb) const noexcept override;
        int FromBBuffer(xx::BBuffer& bb) noexcept override;
        int FromBBufferCore(xx::BBuffer& bb) noexcept;
        inline static std::shared_ptr<ThisType> defaultInstance;
    };
    class UserInfo : public xx::Object
    {
    public:
        int64_t id = 0;
        xx::String_s name;

        typedef UserInfo ThisType;
        typedef xx::Object BaseType;
	    UserInfo() = default;
		UserInfo(UserInfo const&) = delete;
		UserInfo& operator=(UserInfo const&) = delete;
        virtual uint16_t GetTypeId() const noexcept;
        void ToString(std::string& s) const noexcept override;
        void ToStringCore(std::string& s) const noexcept override;
        void ToBBuffer(xx::BBuffer& bb) const noexcept override;
        int FromBBuffer(xx::BBuffer& bb) noexcept override;
        int FromBBufferCore(xx::BBuffer& bb) noexcept;
        inline static std::shared_ptr<ThisType> defaultInstance;
    };
}
namespace Login_DB
{
    // 校验. 成功返回 DB_Login.Auth_Success 内含 userId. 失败返回 Generic.Error
    class Auth : public xx::Object
    {
    public:
        xx::String_s username;
        xx::String_s password;

        typedef Auth ThisType;
        typedef xx::Object BaseType;
	    Auth() = default;
		Auth(Auth const&) = delete;
		Auth& operator=(Auth const&) = delete;
        virtual uint16_t GetTypeId() const noexcept;
        void ToString(std::string& s) const noexcept override;
        void ToStringCore(std::string& s) const noexcept override;
        void ToBBuffer(xx::BBuffer& bb) const noexcept override;
        int FromBBuffer(xx::BBuffer& bb) noexcept override;
        int FromBBufferCore(xx::BBuffer& bb) noexcept;
        inline static std::shared_ptr<ThisType> defaultInstance;
    };
}
namespace DB_Login
{
    // Login_DB.Auth 的成功返回值
    class Auth_Success : public xx::Object
    {
    public:
        int32_t userId = 0;

        typedef Auth_Success ThisType;
        typedef xx::Object BaseType;
	    Auth_Success() = default;
		Auth_Success(Auth_Success const&) = delete;
		Auth_Success& operator=(Auth_Success const&) = delete;
        virtual uint16_t GetTypeId() const noexcept;
        void ToString(std::string& s) const noexcept override;
        void ToStringCore(std::string& s) const noexcept override;
        void ToBBuffer(xx::BBuffer& bb) const noexcept override;
        int FromBBuffer(xx::BBuffer& bb) noexcept override;
        int FromBBufferCore(xx::BBuffer& bb) noexcept;
        inline static std::shared_ptr<ThisType> defaultInstance;
    };
}
namespace Client_Login
{
    // 失败返回 Generic.Error. 成功返回 Login_Client.EnterLobby 或 EnterGame1
    class Auth : public xx::Object
    {
    public:
        xx::String_s username;
        xx::String_s password;

        typedef Auth ThisType;
        typedef xx::Object BaseType;
	    Auth() = default;
		Auth(Auth const&) = delete;
		Auth& operator=(Auth const&) = delete;
        virtual uint16_t GetTypeId() const noexcept;
        void ToString(std::string& s) const noexcept override;
        void ToStringCore(std::string& s) const noexcept override;
        void ToBBuffer(xx::BBuffer& bb) const noexcept override;
        int FromBBuffer(xx::BBuffer& bb) noexcept override;
        int FromBBufferCore(xx::BBuffer& bb) noexcept;
        inline static std::shared_ptr<ThisType> defaultInstance;
    };
}
namespace Login_Client
{
    // 校验成功
    class AuthSuccess : public xx::Object
    {
    public:
        // 连接大厅后要发送的 token
        xx::String_s token;

        typedef AuthSuccess ThisType;
        typedef xx::Object BaseType;
	    AuthSuccess() = default;
		AuthSuccess(AuthSuccess const&) = delete;
		AuthSuccess& operator=(AuthSuccess const&) = delete;
        virtual uint16_t GetTypeId() const noexcept;
        void ToString(std::string& s) const noexcept override;
        void ToStringCore(std::string& s) const noexcept override;
        void ToBBuffer(xx::BBuffer& bb) const noexcept override;
        int FromBBuffer(xx::BBuffer& bb) noexcept override;
        int FromBBufferCore(xx::BBuffer& bb) noexcept;
        inline static std::shared_ptr<ThisType> defaultInstance;
    };
}
}
namespace xx
{
}
namespace PKG
{
namespace Generic
{
    inline uint16_t Success::GetTypeId() const noexcept
    {
        return 3;
    }
    inline void Success::ToBBuffer(xx::BBuffer& bb) const noexcept
    {
    }
    inline int Success::FromBBuffer(xx::BBuffer& bb) noexcept
    {
        return this->FromBBufferCore(bb);
    }
    inline int Success::FromBBufferCore(xx::BBuffer& bb) noexcept
    {
        return 0;
    }

    inline void Success::ToString(std::string& s) const noexcept
    {
        if (this->toStringFlag)
        {
        	xx::Append(s, "[ \"***** recursived *****\" ]");
        	return;
        }
        else this->SetToStringFlag();

        xx::Append(s, "{ \"pkgTypeName\":\"Generic.Success\", \"pkgTypeId\":", GetTypeId());
        ToStringCore(s);
        xx::Append(s, " }");
        
        this->SetToStringFlag(false);
    }
    inline void Success::ToStringCore(std::string& s) const noexcept
    {
        this->BaseType::ToStringCore(s);
    }

    inline uint16_t Error::GetTypeId() const noexcept
    {
        return 4;
    }
    inline void Error::ToBBuffer(xx::BBuffer& bb) const noexcept
    {
        bb.Write(this->number);
        bb.Write(this->text);
    }
    inline int Error::FromBBuffer(xx::BBuffer& bb) noexcept
    {
        return this->FromBBufferCore(bb);
    }
    inline int Error::FromBBufferCore(xx::BBuffer& bb) noexcept
    {
        if (int r = bb.Read(this->number)) return r;
        bb.readLengthLimit = 0;
        if (int r = bb.Read(this->text)) return r;
        return 0;
    }

    inline void Error::ToString(std::string& s) const noexcept
    {
        if (this->toStringFlag)
        {
        	xx::Append(s, "[ \"***** recursived *****\" ]");
        	return;
        }
        else this->SetToStringFlag();

        xx::Append(s, "{ \"pkgTypeName\":\"Generic.Error\", \"pkgTypeId\":", GetTypeId());
        ToStringCore(s);
        xx::Append(s, " }");
        
        this->SetToStringFlag(false);
    }
    inline void Error::ToStringCore(std::string& s) const noexcept
    {
        this->BaseType::ToStringCore(s);
        xx::Append(s, ", \"number\":", this->number);
        if (this->text) xx::Append(s, ", \"text\":\"", this->text, "\"");
        else xx::Append(s, ", \"text\":nil");
    }

    inline uint16_t ServerInfo::GetTypeId() const noexcept
    {
        return 5;
    }
    inline void ServerInfo::ToBBuffer(xx::BBuffer& bb) const noexcept
    {
        bb.Write(this->name);
    }
    inline int ServerInfo::FromBBuffer(xx::BBuffer& bb) noexcept
    {
        return this->FromBBufferCore(bb);
    }
    inline int ServerInfo::FromBBufferCore(xx::BBuffer& bb) noexcept
    {
        bb.readLengthLimit = 0;
        if (int r = bb.Read(this->name)) return r;
        return 0;
    }

    inline void ServerInfo::ToString(std::string& s) const noexcept
    {
        if (this->toStringFlag)
        {
        	xx::Append(s, "[ \"***** recursived *****\" ]");
        	return;
        }
        else this->SetToStringFlag();

        xx::Append(s, "{ \"pkgTypeName\":\"Generic.ServerInfo\", \"pkgTypeId\":", GetTypeId());
        ToStringCore(s);
        xx::Append(s, " }");
        
        this->SetToStringFlag(false);
    }
    inline void ServerInfo::ToStringCore(std::string& s) const noexcept
    {
        this->BaseType::ToStringCore(s);
        if (this->name) xx::Append(s, ", \"name\":\"", this->name, "\"");
        else xx::Append(s, ", \"name\":nil");
    }

    inline uint16_t UserInfo::GetTypeId() const noexcept
    {
        return 6;
    }
    inline void UserInfo::ToBBuffer(xx::BBuffer& bb) const noexcept
    {
        bb.Write(this->id);
        bb.Write(this->name);
    }
    inline int UserInfo::FromBBuffer(xx::BBuffer& bb) noexcept
    {
        return this->FromBBufferCore(bb);
    }
    inline int UserInfo::FromBBufferCore(xx::BBuffer& bb) noexcept
    {
        if (int r = bb.Read(this->id)) return r;
        bb.readLengthLimit = 0;
        if (int r = bb.Read(this->name)) return r;
        return 0;
    }

    inline void UserInfo::ToString(std::string& s) const noexcept
    {
        if (this->toStringFlag)
        {
        	xx::Append(s, "[ \"***** recursived *****\" ]");
        	return;
        }
        else this->SetToStringFlag();

        xx::Append(s, "{ \"pkgTypeName\":\"Generic.UserInfo\", \"pkgTypeId\":", GetTypeId());
        ToStringCore(s);
        xx::Append(s, " }");
        
        this->SetToStringFlag(false);
    }
    inline void UserInfo::ToStringCore(std::string& s) const noexcept
    {
        this->BaseType::ToStringCore(s);
        xx::Append(s, ", \"id\":", this->id);
        if (this->name) xx::Append(s, ", \"name\":\"", this->name, "\"");
        else xx::Append(s, ", \"name\":nil");
    }

}
namespace Login_DB
{
    inline uint16_t Auth::GetTypeId() const noexcept
    {
        return 7;
    }
    inline void Auth::ToBBuffer(xx::BBuffer& bb) const noexcept
    {
        bb.Write(this->username);
        bb.Write(this->password);
    }
    inline int Auth::FromBBuffer(xx::BBuffer& bb) noexcept
    {
        return this->FromBBufferCore(bb);
    }
    inline int Auth::FromBBufferCore(xx::BBuffer& bb) noexcept
    {
        bb.readLengthLimit = 0;
        if (int r = bb.Read(this->username)) return r;
        bb.readLengthLimit = 0;
        if (int r = bb.Read(this->password)) return r;
        return 0;
    }

    inline void Auth::ToString(std::string& s) const noexcept
    {
        if (this->toStringFlag)
        {
        	xx::Append(s, "[ \"***** recursived *****\" ]");
        	return;
        }
        else this->SetToStringFlag();

        xx::Append(s, "{ \"pkgTypeName\":\"Login_DB.Auth\", \"pkgTypeId\":", GetTypeId());
        ToStringCore(s);
        xx::Append(s, " }");
        
        this->SetToStringFlag(false);
    }
    inline void Auth::ToStringCore(std::string& s) const noexcept
    {
        this->BaseType::ToStringCore(s);
        if (this->username) xx::Append(s, ", \"username\":\"", this->username, "\"");
        else xx::Append(s, ", \"username\":nil");
        if (this->password) xx::Append(s, ", \"password\":\"", this->password, "\"");
        else xx::Append(s, ", \"password\":nil");
    }

}
namespace DB_Login
{
    inline uint16_t Auth_Success::GetTypeId() const noexcept
    {
        return 8;
    }
    inline void Auth_Success::ToBBuffer(xx::BBuffer& bb) const noexcept
    {
        bb.Write(this->userId);
    }
    inline int Auth_Success::FromBBuffer(xx::BBuffer& bb) noexcept
    {
        return this->FromBBufferCore(bb);
    }
    inline int Auth_Success::FromBBufferCore(xx::BBuffer& bb) noexcept
    {
        if (int r = bb.Read(this->userId)) return r;
        return 0;
    }

    inline void Auth_Success::ToString(std::string& s) const noexcept
    {
        if (this->toStringFlag)
        {
        	xx::Append(s, "[ \"***** recursived *****\" ]");
        	return;
        }
        else this->SetToStringFlag();

        xx::Append(s, "{ \"pkgTypeName\":\"DB_Login.Auth_Success\", \"pkgTypeId\":", GetTypeId());
        ToStringCore(s);
        xx::Append(s, " }");
        
        this->SetToStringFlag(false);
    }
    inline void Auth_Success::ToStringCore(std::string& s) const noexcept
    {
        this->BaseType::ToStringCore(s);
        xx::Append(s, ", \"userId\":", this->userId);
    }

}
namespace Client_Login
{
    inline uint16_t Auth::GetTypeId() const noexcept
    {
        return 9;
    }
    inline void Auth::ToBBuffer(xx::BBuffer& bb) const noexcept
    {
        bb.Write(this->username);
        bb.Write(this->password);
    }
    inline int Auth::FromBBuffer(xx::BBuffer& bb) noexcept
    {
        return this->FromBBufferCore(bb);
    }
    inline int Auth::FromBBufferCore(xx::BBuffer& bb) noexcept
    {
        bb.readLengthLimit = 0;
        if (int r = bb.Read(this->username)) return r;
        bb.readLengthLimit = 0;
        if (int r = bb.Read(this->password)) return r;
        return 0;
    }

    inline void Auth::ToString(std::string& s) const noexcept
    {
        if (this->toStringFlag)
        {
        	xx::Append(s, "[ \"***** recursived *****\" ]");
        	return;
        }
        else this->SetToStringFlag();

        xx::Append(s, "{ \"pkgTypeName\":\"Client_Login.Auth\", \"pkgTypeId\":", GetTypeId());
        ToStringCore(s);
        xx::Append(s, " }");
        
        this->SetToStringFlag(false);
    }
    inline void Auth::ToStringCore(std::string& s) const noexcept
    {
        this->BaseType::ToStringCore(s);
        if (this->username) xx::Append(s, ", \"username\":\"", this->username, "\"");
        else xx::Append(s, ", \"username\":nil");
        if (this->password) xx::Append(s, ", \"password\":\"", this->password, "\"");
        else xx::Append(s, ", \"password\":nil");
    }

}
namespace Login_Client
{
    inline uint16_t AuthSuccess::GetTypeId() const noexcept
    {
        return 10;
    }
    inline void AuthSuccess::ToBBuffer(xx::BBuffer& bb) const noexcept
    {
        bb.Write(this->token);
    }
    inline int AuthSuccess::FromBBuffer(xx::BBuffer& bb) noexcept
    {
        return this->FromBBufferCore(bb);
    }
    inline int AuthSuccess::FromBBufferCore(xx::BBuffer& bb) noexcept
    {
        bb.readLengthLimit = 0;
        if (int r = bb.Read(this->token)) return r;
        return 0;
    }

    inline void AuthSuccess::ToString(std::string& s) const noexcept
    {
        if (this->toStringFlag)
        {
        	xx::Append(s, "[ \"***** recursived *****\" ]");
        	return;
        }
        else this->SetToStringFlag();

        xx::Append(s, "{ \"pkgTypeName\":\"Login_Client.AuthSuccess\", \"pkgTypeId\":", GetTypeId());
        ToStringCore(s);
        xx::Append(s, " }");
        
        this->SetToStringFlag(false);
    }
    inline void AuthSuccess::ToStringCore(std::string& s) const noexcept
    {
        this->BaseType::ToStringCore(s);
        if (this->token) xx::Append(s, ", \"token\":\"", this->token, "\"");
        else xx::Append(s, ", \"token\":nil");
    }

}
}
namespace PKG
{
	inline void AllTypesRegister() noexcept
	{
	    xx::BBuffer::Register<PKG::Generic::Success>(3);
	    xx::BBuffer::Register<PKG::Generic::Error>(4);
	    xx::BBuffer::Register<PKG::Generic::ServerInfo>(5);
	    xx::BBuffer::Register<PKG::Generic::UserInfo>(6);
	    xx::BBuffer::Register<PKG::Login_DB::Auth>(7);
	    xx::BBuffer::Register<PKG::DB_Login::Auth_Success>(8);
	    xx::BBuffer::Register<PKG::Client_Login::Auth>(9);
	    xx::BBuffer::Register<PKG::Login_Client::AuthSuccess>(10);
	}
}
