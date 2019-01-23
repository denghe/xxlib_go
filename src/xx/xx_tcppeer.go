package xx

import (
	"fmt"
	"net"
	"sync"
	"sync/atomic"
	"time"
)

type TcpPeer struct {
	net.Conn

	readBuf 	[]byte																// for Conn.Read
	recvBuf 	[]byte																// for store buf & Decode
	recvBB		BBuffer																// for decode
	Recvs		[]*NetMessage														// for store decoded packages

	sendBB		BBuffer																// for send

	rpcMutex	sync.Mutex															// for rpcSerials
	rpcSerials	map[int32] chan IObject												// key: serial. value: recv chan
	rpcInc		int32																// atomic++ for gen serial

	status		int32																// atomic update. 0: ok   1: closed
	tag			string																// for debug print info
}

// try read pkg append to Recvs.
func (zs *TcpPeer) Read() (hasError bool) {
	n, err := zs.Conn.Read(zs.readBuf)
	if err != nil {
		e, ok := err.(net.Error)
		if !ok || !e.Temporary() {
			fmt.Println(zs.tag, " conn.Read error: ", err)
			return true
		}
	}
	if n > 0 {
		zs.recvBuf = append(zs.recvBuf, zs.readBuf[:n]...)
		offset := 0

		for ;offset + 3 <= len(zs.recvBuf); {												// ensure header len: 3
			typeId := int32(zs.recvBuf[offset])

			dataLen := int(zs.recvBuf[offset + 1] + (zs.recvBuf[offset + 2] << 8))			// calc data len
			headerLen := 3
			if typeId & 4 > 0 {                       										// ensure length 5 if big pkg
				if offset + 5 > len(zs.recvBuf) {
					return false
				}
				headerLen = 5
				dataLen += int((zs.recvBuf[offset + 3] << 16) + (zs.recvBuf[offset + 4] << 24))	// fix data len for big pkg
			}

			if dataLen <= 0 {
				return true
			}
			if offset + headerLen + dataLen > len(zs.recvBuf) {								// ensure data len
				return false
			}
			offset += headerLen																// jump to data area


			serial := int32(0)
			typeId &= 3
			if typeId == 3 {																// 0, 1, 2 is valid
				return true
			}
			if typeId > 0 {
				serial = int32(zs.recvBB.ReadUInt32())
			}

			zs.recvBB.Offset = offset
			zs.recvBB.Buf = zs.recvBuf
			pkg := zs.recvBB.TryReadRoot()

			if pkg == nil {
				return true
			} else {
				zs.Recvs = append(zs.Recvs, &NetMessage{typeId,serial, pkg })
			}
			offset += dataLen																// jump to next pkg area
		}

		// move left data to top
		leftLen := len(zs.recvBuf) - offset
		if leftLen > 0 {
			copy(zs.recvBuf[:0], zs.recvBuf[offset:])
		}
		zs.recvBuf = zs.recvBuf[:leftLen]
	}
	return false
}

// rpc chan <- typeId == 2 message
func (zs *TcpPeer) HandleResponse(m *NetMessage) {
	v := (chan IObject)(nil)
	found := false
	func(){
		zs.rpcMutex.Lock()
		defer zs.rpcMutex.Unlock()
		v, found = zs.rpcSerials[m.Serial]
	}()
	if found {																		// not found: ignore( timeout )
		select {
		case v <- m.Pkg:
		default:
			close(v)
		}
	}
}

// [typeId] [len] [serial?] [data]
func (zs *TcpPeer) sendCore(typeId uint8, serial int32, pkg IObject) bool {
	zs.sendBB.Clear()
	zs.sendBB.WriteSpaces(5)
	if serial > 0 {
		zs.sendBB.WriteUInt32(uint32(serial))
	}
	zs.sendBB.WriteRoot(pkg)
	p := zs.sendBB.Buf
	dataLen := len(p) - 5
	if dataLen <= 65535	{
		p[2] = typeId
		p[3] = uint8(dataLen)
		p[4] = uint8(dataLen >> 8)
		p = p[2:]
	} else {
		p[0] = typeId | (1 << 2)
		p[1] = uint8(dataLen)
		p[2] = uint8(dataLen >> 8)
		p[3] = uint8(dataLen >> 16)
		p[4] = uint8(dataLen >> 24)
	}
	n, err := zs.Conn.Write(p)
	if err != nil {
		fmt.Println("conn.Write error: ", err)
		return true
	} else if n < len(p) {
		fmt.Println("conn.Write not finished. n = ", n, ", len(p) = ", len(p))
		return true
	}
	return false
}

// [type 0] [len] [data]
func (zs *TcpPeer) Send(pkg IObject) bool {
	return zs.sendCore(0, 0, pkg)
}

// [type 1] [len] [++serial] [data]
func (zs *TcpPeer) SendRequest(pkg IObject, timeout time.Duration) (r IObject) {
	serial := atomic.AddInt32(&zs.rpcInc, 1)
	if !zs.sendCore(1, serial, pkg) {
		return
	}
	if zs.IsClosed() {
		return
	}
	c := make(chan IObject, 1)
	func() {
		zs.rpcMutex.Lock()
		defer zs.rpcMutex.Unlock()
		zs.rpcSerials[serial] = c
	}()
	select {
	case <- time.After(timeout):
	case r = <- c:
	}
	func(){
		zs.rpcMutex.Lock()
		defer zs.rpcMutex.Unlock()
		delete(zs.rpcSerials, serial)
	}()
	return
}

// [type 2] [len] [serial] [data]
func (zs *TcpPeer) SendResponse(serial int32, pkg IObject) bool {
	return zs.sendCore(2, serial, pkg)
}

func (zs *TcpPeer) Close(timeout time.Duration) {
	if atomic.SwapInt32(&zs.status, 1) != 0 {
		return
	}
	if timeout <= 0 {
		_ = zs.Conn.Close()
	} else {
		go func() {
			<- time.After(timeout)
			_ = zs.Conn.Close()
		}()
	}
	func(){
		zs.rpcMutex.Lock()
		defer zs.rpcMutex.Unlock()
		for _, v := range zs.rpcSerials {
			close(v)
		}
	}()
}

func (zs *TcpPeer) IsClosed() bool {
	return atomic.LoadInt32(&zs.status) == 1
}

func (zs *TcpPeer) Alive() bool {
	return atomic.LoadInt32(&zs.status) == 0
}

func NewTcpPeer(conn net.Conn, tag string) *TcpPeer {
	peer := &TcpPeer{}
	peer.Conn = conn
	peer.readBuf = make([]byte, 512)
	peer.recvBuf = make([]byte, 0, 512)
	peer.Recvs = make([]*NetMessage, 0, 16)
	peer.rpcSerials = make(map[int32] chan IObject, 13)
	peer.tag = tag
	return peer
}
