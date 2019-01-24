package bak

import (
	"../xx"
	"fmt"
	"net"
	"time"
)

// 异步狂发狂收

func main() {
	xx.RegisterInternals()

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
}
