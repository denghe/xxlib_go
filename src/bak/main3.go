package bak

import (
	"fmt"
	"net"
	"time"
)


var recvs = make(chan interface{}, 3)

func initListener() {
	listener, err := net.Listen("tcp", ":12345")
	if err != nil {
		panic(err)
	}
	go func() {
		for {
			conn, err := listener.Accept()
			if err != nil {
				fmt.Println("accept error: ", err)
				continue
			}
			go func() {
				defer func() {
					conn.Close()
					fmt.Println("conn.Close.")
				}()
				fmt.Println(conn.RemoteAddr(), " accepted.")
				n := 0
				buf := make([]byte, 10)
				for {
					n, err = conn.Read(buf)
					if err != nil {
						fmt.Println("conn.Read error: ", err)
						goto AfterFor
					} else if n > 0 {
						//fmt.Println(n, buf)
						select {
						case recvs <- buf[0]:
						default:
							goto AfterFor
						}

					}
				}
			AfterFor:
			}()
		}
	}()
}

var frameNumber = 0
func frameUpdate() {
	println("logic: frameNumber = ", frameNumber)

	for {
		select {
		case recv := <-recvs:
			fmt.Println(recv)
		default:
			goto AfterFor
		}
	}
	AfterFor:

	frameNumber++
}

func mainLoop() {
	ticksPerFrame := time.Second	// / 60
	var ticksPool time.Duration
	lastTicks := time.Now()

	for {
		currTicks := time.Now()
		ticksPool += currTicks.Sub(lastTicks)
		lastTicks = currTicks

		for ; ticksPool > ticksPerFrame; {
			frameUpdate()
			ticksPool -= ticksPerFrame
		}

		time.Sleep(time.Microsecond * 1)
	}
}

//func simClient() {
//	conn, err := net.Dial("tcp", "127.0.0.1:12345")
//	if err != nil {
//		fmt.Println("accept error: ", err)
//		return
//	}
//	defer conn.Close()
//	conn.Write([]byte("asdf"))
//}

func main() {
	//go func() {
		initListener()
		mainLoop()
	//}()
	//simClient()
}
