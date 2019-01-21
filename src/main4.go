package main

import (
"./xx"
"fmt"
"net"
"sync"
"sync/atomic"
"time"
)

type NetMessage struct {
	TypeId int32									// 0: Disconnected    1: Package    2: Request
	Serial int32
	Pkg xx.IObject
}

type TcpPeer struct {
	net.Conn
	recvs chan *NetMessage
	rpcMutex sync.Mutex								// for rpcSerials
	rpcSerials map[int32] chan xx.IObject			// serial : recv chan
	rpcInc int32									// atomic++
	status int32									// atomic update		 	0: ok   1: closed
}

func (zs *TcpPeer) PopEvent(timeout time.Duration) (r *NetMessage) {
	if zs.IsClosed() {
		return
	}
	if timeout == 0 {
		select {
		case r = <- zs.recvs:
		default:
		}
	} else if timeout < 0 {
		select {
		case r = <- zs.recvs:
		}
	} else {
		select {
		case r = <- zs.recvs:
		case <- time.After(timeout):
		}
	}
	return
}

func (zs *TcpPeer) Close(timeout time.Duration) {
	if atomic.SwapInt32(&zs.status, 1) != 0 {
		return
	}
	//ra := zs.Conn.RemoteAddr()
	if timeout <= 0 {
		zs.Conn.Close()
	} else {
		go func(conn net.Conn) {
			select {
			case <- time.After(timeout):
				conn.Close()
			}
		}(zs.Conn)
	}
	close(zs.recvs)
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

// [typeId] [len] [serial?] [data]
func (zs *TcpPeer) sendCore(typeId uint8, serial int32, pkg xx.IObject) {
	if zs.IsClosed() {
		return
	}
	defer func() {
		recover()		// todo: handle Conn.Write error
	}()

	bbSend := xx.BBuffer{}
	bbSend.WriteSpaces(5)
	if serial > 0 {
		bbSend.WriteUInt32(uint32(serial))
	}
	bbSend.WriteRoot(pkg)
	p := bbSend.Buf
	dataLen := len(p) - 5
	if dataLen <= 65535	{
		p[2] = typeId
		p[3] = uint8(dataLen)
		p[4] = uint8(dataLen >> 8)
		p = p[2:dataLen + 3]
	} else {
		p[0] = typeId | (1 << 2)
		p[1] = uint8(dataLen)
		p[2] = uint8(dataLen >> 8)
		p[3] = uint8(dataLen >> 16)
		p[4] = uint8(dataLen >> 24)
	}
	zs.Conn.Write(p)
}

// [type 0] [len] [data]
func (zs *TcpPeer) Send(pkg xx.IObject) {
	zs.sendCore(0, 0, pkg)
}

// [type 1] [len] [++serial] [data]
func (zs *TcpPeer) SendRequest(pkg xx.IObject, timeout time.Duration) (r xx.IObject) {
	serial := atomic.AddInt32(&zs.rpcInc, 1)
	zs.sendCore(1, serial, pkg)
	if zs.IsClosed() {
		return
	}

	c := make(chan xx.IObject, 1)
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
func (zs *TcpPeer) SendResponse(serial int32, pkg xx.IObject) {
	zs.sendCore(2, serial, pkg)
}

func (zs *TcpPeer) beginReceive() {
	defer func() {
		recover()																		// for ReadRoot panic
		zs.Close(0)
	}()

	readBuf := make([]byte, 16384)														// for Conn.Read
	bbRecv := xx.BBuffer{}
	buf := make([]byte, 0, 16384)														// for append left data
	for {
		n, err := zs.Conn.Read(readBuf)
		if err != nil {
			fmt.Println("conn.Read error: ", err)
			goto AfterFor
		} else if zs.IsClosed() {
			return
		} else if n > 0 {
			buf = append(buf, readBuf[:n]...)
			offset := 0

			for ;offset + 3 <= len(buf); {												// ensure header len: 3
				typeId := buf[offset]

				dataLen := int(buf[offset + 1] + (buf[offset + 2] << 8))				// calc data len
				headerLen := 3
				if typeId & 4 > 0 {                       								// ensure length 5 if big pkg
					if offset + 5 > len(buf) {
						break
					}
					headerLen = 5
					dataLen += int((buf[offset + 3] << 16) + (buf[offset + 4] << 24))	// fix data len for big pkg
				}

				if dataLen <= 0 {
					goto AfterFor
				}
				if offset + headerLen + dataLen > len(buf) {							// ensure data len
					break
				}
				offset += headerLen														// jump to data area

				bbRecv.Offset = offset													// for read root
				bbRecv.Buf = buf

				pkgType := typeId & 3
				if pkgType == 0 {
					select {
						case zs.recvs <- &NetMessage{ 1, 0, bbRecv.ReadRoot() }:
						default:
							fmt.Println("event chan is full.")
							goto AfterFor
					}
				} else {
					serial := int32(bbRecv.ReadUInt32())
					if pkgType == 1 {
						select {
						case zs.recvs <- &NetMessage{ 2, serial, bbRecv.ReadRoot() }:
						default:
							fmt.Println("event chan is full.")
							goto AfterFor
						}
					} else if pkgType == 2 {
						v := (chan xx.IObject)(nil)
						found := false
						func(){
							zs.rpcMutex.Lock()
							defer zs.rpcMutex.Unlock()
							v, found = zs.rpcSerials[serial]
						}()
						if found {														// not found: ignore( timeout )
							select {
							case v <- bbRecv.ReadRoot():
							default:
								fmt.Println("rpc chan is full.")
								goto AfterFor
							}
						}
					}
				}
				offset += dataLen														// jump to next pkg area
			}

			// move left data to top
			leftLen := len(buf) - offset
			if leftLen > 0 {
				copy(buf[offset:leftLen], buf[:0])
			}
			buf = buf[:leftLen]
		}
	}
AfterFor:
	zs.recvs <- &NetMessage{ 0, 0, nil }							// push disconnect message
}

func newTcpPeer(conn net.Conn, recvsLen int) *TcpPeer {
	peer := &TcpPeer{}
	peer.Conn = conn
	peer.recvs = make(chan *NetMessage, recvsLen)
	go peer.beginReceive()
	return peer
}


func Run() {
	listener, err := net.Listen("tcp", ":12345")
	if err != nil {
		panic(err)
	}
	for {
		conn, err := listener.Accept()
		if err != nil {
			fmt.Println("accept error: ", err)
			continue
		}
		peer := newTcpPeer(conn, 1000)
		go func(){
			fmt.Println("accepted: ", peer.RemoteAddr())
			for {
				e := peer.PopEvent(0)
				if e != nil {
					fmt.Println(e)
				}
			}
		}()
	}
}


func main() {
	Run()
}
