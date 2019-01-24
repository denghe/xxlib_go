package main

import (
	"fmt"
	"net"
	"time"
)

// 最直接的 ping pong 测试

func EchoClient() {
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

			if count++; count == 100000 {
				fmt.Println(time.Now().Sub(t).Seconds())
				return
			}
		}
	}
}

func main() {
	go EchoClient()
	go EchoClient()
	go EchoClient()
	time.Sleep(time.Second * 10)
}
