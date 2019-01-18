package main

import (
	"./xx"
	"fmt"
	"net"
	"sync/atomic"
	"time"
)

// 模拟 listener 产生的 peer
type TcpPeer struct {
	net.Conn

	OnTimeout func()
	OnReceivePackage func(pkg xx.IObject)
	OnReceiveRequest func(serial int32, pkg xx.IObject)
	// todo: OnReceiveRoutingPackage
	Disposed bool

	// todo: 线程安全: lock
	rpcSerials map[int32] chan xx.IObject
	rpcInc int32
}

func (zs *TcpPeer) sendBytes(bytes []uint8) {
	if zs.Disposed {
		panic("TcpPeer is Disposed.")
	}
	n, err := zs.Write(bytes)
	if err != nil {
		panic(err)
	}
	if n < len(bytes) {
		panic("TcpPeer Send is blocked")
	}
}

// [typeId] [len] [serial?] [data]
func (zs *TcpPeer) sendCore(typeId uint8, serial int32, pkg xx.IObject) {
	bbSend := xx.BBuffer{}
	bbSend.WriteSpaces(5)
	if serial >= 0 {
		bbSend.WriteUInt32(uint32(serial))
	}
	bbSend.WriteRoot(pkg)
	p := bbSend.Buf
	dataLen := len(p) - 5
	if dataLen <= 65535	{
		p[2] = typeId
		p[3] = uint8(dataLen)
		p[4] = uint8(dataLen >> 8)
		zs.sendBytes(p[2:dataLen + 3])
	} else {
		p[0] = typeId | (1 << 2)
		p[1] = uint8(dataLen)
		p[2] = uint8(dataLen >> 8)
		p[3] = uint8(dataLen >> 16)
		p[4] = uint8(dataLen >> 24)
		zs.sendBytes(p)
	}
}

// [type 0] [len] [data]
func (zs *TcpPeer) Send(pkg xx.IObject) {
	zs.sendCore(0, -1, pkg)
}

// [type 1] [len] [++serial] [data]
func (zs *TcpPeer) SendRequest(pkg xx.IObject, timeout time.Duration) (r xx.IObject) {
	serial := atomic.AddInt32(&zs.rpcInc, 1)
	zs.sendCore(1, serial, pkg)
	c := make(chan xx.IObject)
	zs.rpcSerials[serial] = c
	t := time.NewTimer(timeout)
	select {
	case <- t.C:
	case r = <- c:
	}
	delete(zs.rpcSerials, serial)
	return
}

// [type 2] [len] [serial] [data]
func (zs *TcpPeer) SendResponse(serial int32, pkg xx.IObject) {
	zs.sendCore(2, serial, pkg)
}

// block receive packages & call OnReceivePackage or OnReceiveRequest or rpcSerials[serial] <- pkg
func (zs *TcpPeer) Receive() {
	// todo: writing

	defer func() {
		zs.Conn.Close()
		zs.Disposed = true
		fmt.Println("conn.Close.")
	}()
	fmt.Println(zs.Conn.RemoteAddr(), " accepted.")


	readBuf := make([]byte, 16384)														// 用于 Conn.Read
	bbRecv := xx.BBuffer{}
	bbRecv.Buf = make([]byte, 16384)													// 用于累积上次剩下的数据
	buf := bbRecv.Buf
	for {
		n, err := zs.Conn.Read(readBuf)
		if err != nil {
			fmt.Println("conn.Read error: ", err)
			goto AfterFor
		} else if n > 0 {
			buf = append(buf, readBuf[:n]...)											// 追加到累积 buf 以简化流程
			offset := 0																	//

			for ;offset + 3 <= len(buf); {												// 确保 3 字节 包头长度
				typeId := buf[offset]                   								// 读出头

				dataLen := int(buf[offset + 1] + (buf[offset + 2] << 8))				// 读出包长
				headerLen := 3
				if typeId & 4 > 0 {                       								// 大包确保 5 字节 包头长度
					if offset + 5 > len(buf) {
						break
					}
					headerLen = 5
					dataLen += int((buf[offset + 3] << 16) + (buf[offset + 4] << 24))	// 修正为大包包长
				}

				if dataLen <= 0 {                           							// 数据异常判断
					goto AfterFor
				}
				if offset + headerLen + dataLen > len(buf) {							// 确保数据长
					break
				}
				//pkgOffset := offset	// for router
				offset += headerLen

				if typeId & 8 > 0 {														// 转发类数据包
					goto AfterFor
					// todo
					//addrLen := typeId >> 4
					//addrOffset := offset
					//pkgLen := offset + dataLen - pkgOffset
					//offset += addrLen
					//zs.bbRecv.Offset = offset;
					//if zs.OnReceiveRoutingPackage != nil {
					//// todo: 解包
					//	//OnReceiveRoutingPackage(bbRecv, pkgOffset, pkgLen, addrOffset, addrLen);
					//}
				} else {
					bbRecv.Offset = offset												// 准备 ReadRoot
					bbRecv.Buf = buf

					pkgType := typeId & 3
					if pkgType == 0 {
						if zs.OnReceivePackage != nil {
							zs.OnReceivePackage(bbRecv.ReadRoot())
						}
					} else {
						serial := int32(bbRecv.ReadUInt32())
						if pkgType == 1 {
							if zs.OnReceiveRequest != nil {
								zs.OnReceiveRequest(serial, bbRecv.ReadRoot())
							}
						} else if pkgType == 2 {
							if v, found := zs.rpcSerials[serial]; found {
								v <- bbRecv.ReadRoot()
							}
						}
					}
				}
				if zs.Disposed || len(bbRecv.Buf) == 0 /* || Disconnected */ {
					goto AfterFor
				}
				offset += dataLen														// 继续处理剩余数据
			}

			leftLen := len(buf) - offset
			if leftLen > 0 {															// 还有剩余的数据: 移到最前面
				copy(buf[offset:leftLen], buf[:0])
			}
			buf = buf[:leftLen]
		}
	}
AfterFor:
}

func main() {
	bb := xx.BBuffer{}
	fmt.Println(bb)
}
