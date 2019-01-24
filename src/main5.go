package main

import (
	"./xx"
	"net"
	"time"
)

// 简单业务模拟. 身份状态转变, 匿名用户通过登录变为普通用户.


// 简单模拟生成物. 命名结构: Template_From_To_PackageName


// 登陆服务器 校验包 首包
type PKG_Client_Login_Auth struct {
	Username xx.NullableString
	Password xx.NullableString
}
func Read_PKG_Client_Login_Auth(zs *xx.BBuffer) *PKG_Client_Login_Auth {
	v := zs.ReadIObject()
	if v == nil {
		return nil
	} else {
		return v.(*PKG_Client_Login_Auth)
	}
}
func (zs *PKG_Client_Login_Auth) GetPackageId() uint16 {
	return uint16(3)
}
func (zs *PKG_Client_Login_Auth) ToBBuffer(bb *xx.BBuffer) {
	bb.WriteNullableString(zs.Username)
	bb.WriteNullableString(zs.Password)
}
func (zs *PKG_Client_Login_Auth) FromBBuffer(bb *xx.BBuffer) {
	zs.Username = bb.ReadNullableString()
	zs.Password = bb.ReadNullableString()
}



// 校验成功, 返回用于登陆游戏服务器的 token
type PKG_Login_Client_AuthSuccess struct {
	Token xx.NullableString
}
func Read_PKG_Login_Client_AuthSuccess(zs *xx.BBuffer) *PKG_Login_Client_AuthSuccess {
	v := zs.ReadIObject()
	if v == nil {
		return nil
	} else {
		return v.(*PKG_Login_Client_AuthSuccess)
	}
}
func (zs *PKG_Login_Client_AuthSuccess) GetPackageId() uint16 {
	return uint16(4)
}
func (zs *PKG_Login_Client_AuthSuccess) ToBBuffer(bb *xx.BBuffer) {
	bb.WriteNullableString(zs.Token)
}
func (zs *PKG_Login_Client_AuthSuccess) FromBBuffer(bb *xx.BBuffer) {
	zs.Token = bb.ReadNullableString()
}




// 通用成功返回
type PKG_Generic_Success struct {
}
func Read_PKG_Generic_Success(zs *xx.BBuffer) *PKG_Generic_Success {
	v := zs.ReadIObject()
	if v == nil {
		return nil
	} else {
		return v.(*PKG_Generic_Success)
	}
}
func (zs *PKG_Generic_Success) GetPackageId() uint16 {
	return uint16(5)
}
func (zs *PKG_Generic_Success) ToBBuffer(bb *xx.BBuffer) {
}
func (zs *PKG_Generic_Success) FromBBuffer(bb *xx.BBuffer) {
}




// 通用错误返回
type PKG_Generic_Error struct {
	Number int32
	Text xx.NullableString
}
func Read_PKG_Generic_Error(zs *xx.BBuffer) *PKG_Generic_Error {
	v := zs.ReadIObject()
	if v == nil {
		return nil
	} else {
		return v.(*PKG_Generic_Error)
	}
}
func (zs *PKG_Generic_Error) GetPackageId() uint16 {
	return uint16(6)
}
func (zs *PKG_Generic_Error) ToBBuffer(bb *xx.BBuffer) {
	bb.WriteInt32(zs.Number)
	bb.WriteNullableString(zs.Text)
}
func (zs *PKG_Generic_Error) FromBBuffer(bb *xx.BBuffer) {
	zs.Number = bb.ReadInt32()
	zs.Text = bb.ReadNullableString()
}



// 客户端向游戏服务器发送 token 进入服务器. 成功返回 Success
type PKG_Client_Server_Enter struct {
	Token xx.NullableString
}
func Read_PKG_Client_Server_Enter(zs *xx.BBuffer) *PKG_Client_Server_Enter {
	v := zs.ReadIObject()
	if v == nil {
		return nil
	} else {
		return v.(*PKG_Client_Server_Enter)
	}
}
func (zs *PKG_Client_Server_Enter) GetPackageId() uint16 {
	return uint16(7)
}
func (zs *PKG_Client_Server_Enter) ToBBuffer(bb *xx.BBuffer) {
	bb.WriteNullableString(zs.Token)
}
func (zs *PKG_Client_Server_Enter) FromBBuffer(bb *xx.BBuffer) {
	zs.Token = bb.ReadNullableString()
}




// 进入服务器后通过 Ping 保持长连接
type PKG_Generic_Ping struct {
	NanoTicks uint64
}
func Read_PKG_Generic_Ping(zs *xx.BBuffer) *PKG_Generic_Ping {
	v := zs.ReadIObject()
	if v == nil {
		return nil
	} else {
		return v.(*PKG_Generic_Ping)
	}
}
func (zs *PKG_Generic_Ping) GetPackageId() uint16 {
	return uint16(8)
}
func (zs *PKG_Generic_Ping) ToBBuffer(bb *xx.BBuffer) {
	bb.WriteUInt64(zs.NanoTicks)
}
func (zs *PKG_Generic_Ping) FromBBuffer(bb *xx.BBuffer) {
	zs.NanoTicks = bb.ReadUInt64()
}




// 登陆服务器通知游戏服务器有玩家进入
type PKG_Login_Server_Enter struct {
	Id int32
}
func Read_PKG_Login_Server_Enter(zs *xx.BBuffer) *PKG_Login_Server_Enter {
	v := zs.ReadIObject()
	if v == nil {
		return nil
	} else {
		return v.(*PKG_Login_Server_Enter)
	}
}
func (zs *PKG_Login_Server_Enter) GetPackageId() uint16 {
	return uint16(9)
}
func (zs *PKG_Login_Server_Enter) ToBBuffer(bb *xx.BBuffer) {
	bb.WriteInt32(zs.Id)
}
func (zs *PKG_Login_Server_Enter) FromBBuffer(bb *xx.BBuffer) {
	zs.Id = bb.ReadInt32()
}






// 游戏服务器给登陆服务器返回进入成功, 附带 token
type PKG_Server_Login_EnterSuccess struct {
	Token xx.NullableString
}
func Read_PKG_Server_Login_EnterSuccess(zs *xx.BBuffer) *PKG_Server_Login_EnterSuccess {
	v := zs.ReadIObject()
	if v == nil {
		return nil
	} else {
		return v.(*PKG_Server_Login_EnterSuccess)
	}
}
func (zs *PKG_Server_Login_EnterSuccess) GetPackageId() uint16 {
	return uint16(10)
}
func (zs *PKG_Server_Login_EnterSuccess) ToBBuffer(bb *xx.BBuffer) {
	bb.WriteNullableString(zs.Token)
}
func (zs *PKG_Server_Login_EnterSuccess) FromBBuffer(bb *xx.BBuffer) {
	zs.Token = bb.ReadNullableString()
}




// todo


func RunLogin() {
	listener, _ := net.Listen("tcp", ":10001")
	// todo: user map
	for {
		c_, _ := listener.Accept()
		c := xx.NewTcpPeer(c_)
		c.Tag = "server"
		go func() {
			defer c.Close(0)
			if c.SetTimeout(time.Second * 20) {
				return
			}
			for {
				if c.Read() {
					return
				}
				for _, msg := range c.Recvs {
					if msg.TypeId != 2 {			// 收到非 rpc 包直接断开
						return
					}
					switch msg.Pkg.(type) {
					case *PKG_Client_Login_Auth:
						recv := msg.Pkg.(*PKG_Client_Login_Auth)
						// 基础前置检查
						if !recv.Username.HasValue || !recv.Password.HasValue || len(recv.Username.Value) == 0 {
							return
						}
						// 逻辑模拟
						if recv.Username.Value == "abc" && recv.Password.Value == "123" {
							pkg := &PKG_Login_Client_AuthSuccess {
								xx.NullableString{ "asdfasdfasdfasdf", true } }
							if c.Send(pkg) {
								return
							}
						} else {
							pkg := &PKG_Generic_Error{
								-1,
								xx.NullableString{ "invalid username or password.", true } }
							if c.Send(pkg) {
								return
							}
						}
					default:
						return						// 收到不认识的包直接断开
					}
				}
				c.Recvs = c.Recvs[:0]
			}
		}()
	}
}

func RunServer() {
	listener, _ := net.Listen("tcp", ":10002")

	// todo: user map

	for {
		c_, _ := listener.Accept()
		c := xx.NewTcpPeer(c_)
		c.Tag = "server"
		go func() {
			defer c.Close(0)
			if c.SetTimeout(time.Second * 20) {
				return
			}
			for {
				if c.Read() {
					return
				}
				for _, msg := range c.Recvs {
					if msg.TypeId != 2 {			// 收到非 rpc 包直接断开
						return
					}
					switch msg.Pkg.(type) {
					case *PKG_Client_Login_Auth:
						recv := msg.Pkg.(*PKG_Client_Login_Auth)
						// 基础前置检查
						if !recv.Username.HasValue || !recv.Password.HasValue || len(recv.Username.Value) == 0 {
							return
						}
						// 逻辑模拟
						if recv.Username.Value == "abc" && recv.Password.Value == "123" {
							pkg := &PKG_Login_Client_AuthSuccess {
								xx.NullableString{ "asdfasdfasdfasdf", true } }
							if c.Send(pkg) {
								return
							}
						} else {
							pkg := &PKG_Generic_Error{
								-1,
								xx.NullableString{ "invalid username or password.", true } }
							if c.Send(pkg) {
								return
							}
						}
					default:
						return						// 收到不认识的包直接断开
					}
				}
				c.Recvs = c.Recvs[:0]
			}
		}()
	}
}

func RunClient() {
	c_, _ := net.Dial("tcp", ":10001")
	c := xx.NewTcpPeer(c_)
	c.Tag = "client"
	func() {
		defer c.Close(0)
		for {
			if c.IsClosed() {
				return
			}
			// todo
		}
	}()
}

func main() {
	xx.RegisterInternals()
	//go Server()
	//Client()
	//time.Sleep(time.Second * 10)
}
