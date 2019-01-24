package bak

import (
	"../xx"
	"net"
	"time"
)

// 异步狂发狂收

func main() {
	xx.RegisterInternals()

	f := func() {
		c_, _ := net.Dial("tcp", ":12345")
		c := xx.NewTcpPeer(c_)
		c.Tag = "client"
		bb := xx.BBuffer{}
		for i := 0; i < 500; i++ {
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
	go f()
	time.Sleep(time.Second * 10)
}
