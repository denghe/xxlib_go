package main

import (
	"net"
)

// 最直接的 ping pong 测试

func EchoServer() {
	listener, _ := net.Listen("tcp", ":12345")
	for {
		c, _ := listener.Accept()
		go func() {
			defer func() {
				_ = c.Close()
			}()
			buf := make([]byte, 10)
			for {
				n, err := c.Read(buf)
				if err != nil {
					e, ok := err.(net.Error)
					if !ok || !e.Temporary() {
						return
					}
				}
				if n > 0 {
					p := buf[:n]
				WriteAgain:
					n, err = c.Write(p)
					if err != nil {
						if e, ok := err.(net.Error); !ok || !e.Temporary() {
							return
						}
					}
					if n < len(p) {
						p = p[n:]
						goto WriteAgain
					}
				}
			}
		}()
	}
}

func main() {
	EchoServer()
}
