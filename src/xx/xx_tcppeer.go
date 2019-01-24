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

	sends		chan []byte															// send queue

	rpcMutex	sync.Mutex															// for rpcSerials
	rpcSerials	map[int32] chan IObject												// key: serial. value: recv chan
	rpcInc		int32																// atomic++ for gen serial

	OnClose		func()																// Closing callback
	Tag			string																// for debug print info
	UserData	interface{}															// can store some user data
	status		int32																// atomic update. 0: ok   1: closed
}

// try read pkg append to Recvs.
func (zs *TcpPeer) Read() (hasError bool) {
	defer func() {
		if err := recover(); err != nil {
			fmt.Println("Read() error: ", err)
			hasError = true
		}
	}()
	n, err := zs.Conn.Read(zs.readBuf)
	if err != nil {
		e, ok := err.(net.Error)
		if !ok || !e.Temporary() {
			fmt.Println(zs.Tag, "conn.Read error:", err)
			return true
		}
	}
	if n > 0 {
		zs.recvBuf = append(zs.recvBuf, zs.readBuf[:n]...)
		offset := 0

		for ;offset + 3 <= len(zs.recvBuf); {												// ensure header len: 3
			typeId := int32(zs.recvBuf[offset])

			dataLen := int(zs.recvBuf[offset + 1]) + (int(zs.recvBuf[offset + 2]) << 8)		// calc data len
			headerLen := 3
			if typeId & 4 > 0 {                       										// ensure length 5 if big pkg
				if offset + 5 > len(zs.recvBuf) {
					return false
				}
				headerLen = 5
				dataLen += int((zs.recvBuf[offset + 3]) << 16) + (int(zs.recvBuf[offset + 4] << 24))	// fix data len for big pkg
			}

			if dataLen <= 0 {
				return true
			}
			if offset + headerLen + dataLen > len(zs.recvBuf) {								// ensure data len
				return false
			}
			offset += headerLen																// jump to data area

			zs.recvBB.Offset = offset
			zs.recvBB.Buf = zs.recvBuf

			serial := int32(0)
			typeId &= 3
			if typeId == 3 {																// 0, 1, 2 is valid
				return true
			}
			if typeId > 0 {
				serial = int32(zs.recvBB.ReadUInt32())
			}

			pkg := zs.recvBB.ReadRoot()
			zs.Recvs = append(zs.Recvs, &NetMessage{typeId,serial, pkg })

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
	if zs.IsClosed() {
		return true
	}
	bb := BBuffer{}
	bb.WriteSpaces(5)
	if serial > 0 {
		bb.WriteUInt32(uint32(serial))
	}
	bb.WriteRoot(pkg)
	p := bb.Buf
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
	return func() (r bool) {
		defer func() {
			if err := recover(); err != nil {
				fmt.Println("sendCore error:", err)
			}
			r = true
		}()
		select {
		case zs.sends <- p:
			r = false
		default:
			r = true
		}
		return
	}()
}

// [type 0] [len] [data]
func (zs *TcpPeer) Send(pkg IObject) bool {
	return zs.sendCore(0, 0, pkg)
}

// [type 1] [len] [++serial] [data]
func (zs *TcpPeer) SendRequest(pkg IObject, timeout time.Duration) (r IObject) {
	serial := atomic.AddInt32(&zs.rpcInc, 1)
	if zs.sendCore(1, serial, pkg) {
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

func (zs *TcpPeer) beginSend() {
	for {
		buf := <- zs.sends
		if buf == nil || zs.IsClosed() {
			return
		}

		// todo: 包合并？

	TrySend:
		n, err := zs.Conn.Write(buf)
		if err != nil {
			if e, ok := err.(net.Error); !ok || !e.Temporary() {
				fmt.Println(zs.Tag, "conn.Write error:", err)
				zs.Close(0)
				return
			}
		}
		if n < len(buf) {
			buf = buf[n:]
			goto TrySend
		}
	}
}

func (zs *TcpPeer) Close(timeout time.Duration) {
	if atomic.SwapInt32(&zs.status, 1) != 0 {
		return
	}
	if zs.OnClose != nil {
		zs.OnClose()
	}
	if timeout <= 0 {
		_ = zs.Conn.Close()
		close(zs.sends)
	} else {
		go func() {
			<- time.After(timeout)
			_ = zs.Conn.Close()
			close(zs.sends)
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

func NewTcpPeer(conn net.Conn) *TcpPeer {
	peer := &TcpPeer{}
	peer.Conn = conn
	peer.readBuf = make([]byte, 65535)
	peer.recvBuf = make([]byte, 0, 65535)
	peer.Recvs = make([]*NetMessage, 0, 2048)
	peer.rpcSerials = make(map[int32] chan IObject, 13)

	peer.sends = make(chan []byte, 1024)
	go peer.beginSend()
	return peer
}
