package main

import (
	"./xx"
	"fmt"
	"net"
	"sync"
	"time"
)


type NetMessages struct {
	mutex sync.Mutex
	recvs []*xx.NetMessage
}
func (zs *NetMessages) Push(m *xx.NetMessage) {
	zs.mutex.Lock()
	defer zs.mutex.Unlock()
	zs.recvs = append(zs.recvs, m)
}
func (zs *NetMessages) PopAll() (r []*xx.NetMessage) {
	zs.mutex.Lock()
	defer zs.mutex.Unlock()
	if len(zs.recvs) == 0 {
		return
	}
	r = zs.recvs
	zs.recvs = make([]*xx.NetMessage, 0, cap(r))
	return
}



type MyConn struct {
	net.Conn
	readBuf []byte
	recvBuf []byte
	Recvs   NetMessages
	bb      xx.BBuffer
}

func NewMyConn(conn net.Conn, readBufLen int, recvBufLen int) *MyConn {
	peer := &MyConn{}
	peer.Conn = conn
	peer.readBuf = make([]byte, readBufLen)
	peer.recvBuf = make([]byte, 0, recvBufLen)
	return peer
}

func (zs *MyConn) Read() (r int) {
	n, err := zs.Conn.Read(zs.readBuf)
	if err != nil {
		fmt.Println("conn.Read error: ", err)
		goto AfterFor
	} else if n > 0 {
		zs.recvBuf = append(zs.recvBuf, zs.readBuf[:n]...)
		offset := 0

		for ;offset + 3 <= len(zs.recvBuf); {												// ensure header len: 3
			typeId := zs.recvBuf[offset]

			dataLen := int(zs.recvBuf[offset + 1] + (zs.recvBuf[offset + 2] << 8))			// calc data len
			headerLen := 3
			if typeId & 4 > 0 {                       										// ensure length 5 if big pkg
				if offset + 5 > len(zs.recvBuf) {
					break
				}
				headerLen = 5
				dataLen += int((zs.recvBuf[offset + 3] << 16) + (zs.recvBuf[offset + 4] << 24))	// fix data len for big pkg
			}

			if dataLen <= 0 {
				goto AfterFor
			}
			if offset + headerLen + dataLen > len(zs.recvBuf) {								// ensure data len
				break
			}
			offset += headerLen																// jump to data area

			zs.bb.Offset = offset // for read root
			zs.bb.Buf = zs.recvBuf

			pkgType := typeId & 3
			if pkgType == 0 {
				zs.Recvs.Push(&xx.NetMessage{0, zs.bb.TryReadRoot() })
			} else {
				serial := int32(zs.bb.ReadUInt32())
				if pkgType == 1 {
					zs.Recvs.Push(&xx.NetMessage{serial, zs.bb.TryReadRoot() })
				} else if pkgType == 2 {
					// todo
				}
			}
			r++
			offset += dataLen																// jump to next pkg area
		}

		// move left data to top
		leftLen := len(zs.recvBuf) - offset
		if leftLen > 0 {
			copy(zs.recvBuf[:0], zs.recvBuf[offset:])
		}
		zs.recvBuf = zs.recvBuf[:leftLen]
	}
	return
AfterFor:
	// todo; clean up
	return -1
}

// [typeId] [len] [serial?] [data]
func (zs *MyConn) sendCore(typeId uint8, serial int32, pkg xx.IObject) {
	defer func() {
		recover()		// todo: handle Conn.Write error
	}()

	zs.bb.Clear()
	zs.bb.WriteSpaces(5)
	if serial > 0 {
		zs.bb.WriteUInt32(uint32(serial))
	}
	zs.bb.WriteRoot(pkg)
	p := zs.bb.Buf
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
	} else if n < len(p) {
		fmt.Println("conn.Write not finished. n = ", n, ", len(p) = ", len(p))
	}
}

// [type 0] [len] [data]
func (zs *MyConn) Send(pkg xx.IObject) {
	zs.sendCore(0, 0, pkg)
}

func main() {
	xx.RegisterInternals()

	go func() {
		listener, _ := net.Listen("tcp", ":12345")
		for {
			c_, _ := listener.Accept()
			c := NewMyConn(c_, 512, 512)
			go func() {
				for {
					r := c.Read()
					if r == 0 {
						continue
					} else if r < 0 {
						fmt.Println("server peer read error")
						break
					} else {
						// echo
						recvs := c.Recvs.PopAll()
						for _, v := range recvs {
							c.Send(v.Pkg)
						}
					}
				}
			}()
		}
	}()

	f := func() {
		c_, _ := net.Dial("tcp", ":12345")
		c := NewMyConn(c_, 512, 512)
		bb := xx.BBuffer{}
		bb.Buf = append(bb.Buf, []byte{1, 2, 3, 4, 5, 6}...)
		c.Send(&bb)
		count := 0
		t := time.Now()
		for {
			r := c.Read()
			if r == 0 {
				continue
			}
			if r < 0 {
				fmt.Println("server peer read error")
				break
			}
			// echo
			recvs := c.Recvs.PopAll()
			for _, v := range recvs {
				c.Send(v.Pkg)
				count++
				if count == 10000 {
					goto AfterFor
				}
			}
		}
	AfterFor:
		fmt.Println(time.Now().Sub(t).Seconds())
	}
	go f()
	go f()
	go f()
	go f()
	go f()
	time.Sleep(time.Second * 5)
}


/*
func main() {
	go func(){
		listener, _ := net.Listen("tcp", ":12345")
		for {
			conn, _ := listener.Accept()
			go func(){
				buf := make([]byte, 1024)
				for {
					n, _ := conn.Read(buf)
					if n > 0 {
						_, _ = conn.Write(buf[:n])
					}
				}
			}()
		}
	}()

	f := func() {
		conn, _ := net.Dial("tcp", ":12345")
		buf := make([]byte, 1024)
		_, _ = conn.Write(buf[:10])
		count := 0
		siz := 0
		t := time.Now()
		for {
			n, _ := conn.Read(buf)
			if n > 0 {
				count++
				siz += n
				if count == 100000 {
					break
				}
				_, _ = conn.Write(buf[:n])
			}
		}
		fmt.Println(time.Now().Sub(t).Seconds(), " ", siz)
	}
	//go f()
	//go f()
	//time.Sleep(time.Second * 5)
	f()
}
*/