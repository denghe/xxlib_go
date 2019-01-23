package main

import (
	"fmt"
	"net"
	"time"
)

// 最直接的 ping pong 测试

func Server() {
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
					count := n
					n, err = c.Write(buf[:n])
					if err != nil || n != count {
						return
					}
				}
			}
		}()
	}
}

func Client() {
	count := 0
	t := time.Now()

	c, _ := net.Dial("tcp", ":12345")
	_, _ = c.Write([]byte{1})

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
			n_ := n
			if n, err = c.Write(buf[:n]); err != nil || n != n_ {
				return
			}

			if count++; count == 100000 {
				fmt.Println(time.Now().Sub(t).Seconds())
				return
			}
		}
	}
}

func main() {
	go Server()
	Client()
}
