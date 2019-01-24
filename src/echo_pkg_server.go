package main

import (
	"./xx"
	"net"
)

func main() {
	xx.RegisterInternals()

	listener, _ := net.Listen("tcp", ":12345")
	for {
		c_, _ := listener.Accept()
		c := xx.NewTcpPeer(c_)
		c.Tag = "server"
		go func() {
			defer c.Close(0)
			for {
				if c.Read() {
					return
				}
				for _, m := range c.Recvs {
					switch m.TypeId {
					case 0:								// push
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
	}
}
