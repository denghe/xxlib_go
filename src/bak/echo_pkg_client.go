package bak

import (
	"../xx"
	"fmt"
	"net"
	"time"
)

func main() {
	xx.RegisterInternals()

	f := func() {
		c_, _ := net.Dial("tcp", ":12345")
		c := xx.NewTcpPeer(c_)
		c.Tag = "client"
		bb := xx.BBuffer{}
		bb.Buf = append(bb.Buf, []byte{1, 2, 3, 4, 5, 6}...)
		c.Send(&bb)
		count := 0
		t := time.Now()
		func() {
			defer c.Close(0)
			for {
				if c.Read() {
					return
				}
				for _, m := range c.Recvs {
					switch m.TypeId {
					case 0:								// push
						count++
						if count == 100000 {
							return
						}
						c.Send(m.Pkg)					// echo test
					case 1:								// request
						// todo
					case 2:								// response
						c.HandleResponse(m)
					}
				}
				c.Recvs = c.Recvs[:0]
			}
		}()
		fmt.Println(time.Now().Sub(t).Seconds(), " count == ", count)
	}
	go f()
	//go f()
	//go f()
	time.Sleep(time.Second * 10)
}
