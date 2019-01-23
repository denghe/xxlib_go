package main

import (
	"./xx"
	"fmt"
	"net"
	"time"
)

func main() {
	xx.RegisterInternals()

	go func() {
		listener, _ := net.Listen("tcp", ":12345")
		for {
			c_, _ := listener.Accept()
			c := xx.NewTcpPeer(c_, "server")
			go func() {
				defer c.Close(0)
				for {
					if c.Read() {
						return
					}
					for _, m := range c.Recvs {
						switch m.TypeId {
						case 0:								// push
							if c.Send(m.Pkg) {				// echo test
								return
							}
						case 1:								// request
							// todo
						case 2:								// response
							c.HandleResponse(m)
						}
					}
					c.Recvs = c.Recvs[:0]
				}
			}()
		}
	}()

	f := func() {
		c_, _ := net.Dial("tcp", ":12345")
		c := xx.NewTcpPeer(c_, "client")
		bb := xx.BBuffer{}
		bb.Buf = append(bb.Buf, []byte{1, 2, 3, 4, 5, 6}...)
		c.Send(&bb)
		count := 0
		t := time.Now()
		for {
			if c.Read() {
				goto AfterFor
			}
			// echo
			if len(c.Recvs) != 0 {
				for _, m := range c.Recvs {
					count++
					if count == 100000 {
						goto AfterFor
					}
					if c.Send(m.Pkg) {
						goto AfterFor
					}
				}
				c.Recvs = c.Recvs[:0]
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
	go f()
	go f()
	go f()
	time.Sleep(time.Second * 10)
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