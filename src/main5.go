package main

import (
	"./xx"
	"fmt"
	"net"
	"time"
)

// 异步狂发狂收

func main() {
	xx.RegisterInternals()

	go func() {
		listener, _ := net.Listen("tcp", ":12345")
		for {
			count := 0
			c_, _ := listener.Accept()
			c := xx.NewTcpPeer(c_)
			c.Tag = "server"
			t := time.Now()
			go func() {
				defer c.Close(0)
				for {
					if c.Read() {
						return
					}
					count += len(c.Recvs)
					c.Recvs = c.Recvs[:0]

					if count >= 1000000 {
						fmt.Println(time.Now().Sub(t).Seconds(), " count == ", count)
						return
					}
				}
			}()
		}
	}()

	f := func() {
		c_, _ := net.Dial("tcp", ":12345")
		c := xx.NewTcpPeer(c_)
		c.Tag = "client"
		bb := xx.BBuffer{}
		for i := 0; i < 50; i++ {
			bb.Buf = append(bb.Buf, []byte{1, 2, 3, 4, 5, 6, 7, 8, 9, 0}...)
		}
		c.Send(&bb)
		func() {
			defer c.Close(0)
			for {
				if c.IsClosed() {
					return
				}
				if c.Send(&bb) {
					continue
				}
			}
		}()
	}
	go f()
	go f()
	go f()
	time.Sleep(time.Second * 10)
}
