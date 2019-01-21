package main

import (
	"./xx"
	"fmt"
	"net"
)

func TcpListen(addr string) {
	listener, err := net.Listen("tcp", addr)
	if err != nil {
		panic(err)
	}
	for {
		conn, err := listener.Accept()
		if err != nil {
			fmt.Println("listener accept error: ", err)
			continue
		}
		peer := xx.NewTcpPeer(conn, 1000)
		go func(){
			fmt.Println("listener accepted: ", peer.RemoteAddr())
			for {
				e := peer.PopEvent()
				if e == nil {
					fmt.Println("peer disconnected: ", peer.RemoteAddr())
					break
				}
				fmt.Println("peer pop event: ", e)
				// todo: logic here
			}
		}()
	}
}

func main() {
	xx.RegisterInternals()
	bb := &xx.BBuffer{}
	bb.WriteUInt32(1)

	go TcpListen(":12345")

	// test client
	conn, _ := net.Dial("tcp", ":12345")
	peer := xx.NewTcpPeer(conn, 1000)
	peer.Send(bb)
	for {
		e := peer.PopEvent()
		if e == nil {
			fmt.Println("client disconnected: ", peer.RemoteAddr())
			break
		}
		fmt.Println("client pop event: ", e)
		// todo: logic here
	}
}
