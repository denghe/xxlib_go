package main

import (
	"./xx"
	"fmt"
	"net"
	"time"
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
				//fmt.Println("peer pop event: ", e)
				// echo
				if e.Serial == 0 {
					peer.Send(e.Pkg)
				} else {
					peer.SendResponse(e.Serial, e.Pkg)
				}
			}
		}()
	}
}

func main() {
	xx.RegisterInternals()

	go TcpListen(":12345")

	// test client
	conn, _ := net.Dial("tcp", ":12345")
	peer := xx.NewTcpPeer(conn, 1000)
	bb := xx.BBuffer{}
	bb.WriteUInt8(123)
	peer.Send(&bb)
	n := 0
	t := time.Now()
	for {
		e := peer.PopEvent()
		if e == nil {
			fmt.Println("client disconnected: ", peer.RemoteAddr())
			break
		}
		// echo
		if e.Serial == 0 {
			peer.Send(e.Pkg)
		} else {
			peer.SendResponse(e.Serial, e.Pkg)
		}
		n++
		if n == 1000000 {
			break
		}
	}
	fmt.Println(time.Now().Sub(t).Seconds())
}
