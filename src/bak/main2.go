package bak

import (
	"fmt"
	"net"
	"sync/atomic"
	"time"
)

// 模拟 rpc

var recvSerial = int32(0)

type TcpPeer struct {
	net.Conn
	rpcs map[int32] chan interface{}
	rpcInc int32
	disposed bool
}

func (zs *TcpPeer) Init() {
	zs.rpcs = make(map[int32] chan interface{})
}

func (zs *TcpPeer) SendXxx(pkg interface{}, timeout time.Duration) (r interface{}) {
	serial := atomic.AddInt32(&zs.rpcInc, int32(1))

	// sim send
	recvSerial = serial

	c := make(chan interface{})
	zs.rpcs[serial] = c
	t := time.NewTimer(timeout)
	select {
		case <- t.C:
		case r = <- c:
	}
	delete(zs.rpcs, serial)
	return
}

// sim recv
func (zs *TcpPeer) RecvXxx(serial int32, pkg interface{}) {
	if v, found := zs.rpcs[serial]; found {
		v <- pkg
	}
}

func main() {

	// sim got conn
	peer := &TcpPeer{}
	peer.Init()

	go func() {
		rtv := peer.SendXxx(nil, time.Second)
		fmt.Println(rtv)
	}()

	go func() {
		peer.RecvXxx(recvSerial, 123)
		time.Sleep(time.Millisecond * 500)
		// sim recv
	}()

	time.Sleep(time.Second * 2)
}







//
//input := make(chan interface{})
//
////producer - produce the messages
//go func() {
//	for i := 0; i < 5; i++ {
//		input <- i
//	}
//	input <- "hello, world"
//}()
//
//t1 := time.NewTimer(time.Second * 5)
//t2 := time.NewTimer(time.Second * 10)
//
//for {
//select {
////consumer - consume the messages
//case msg := <-input:
//fmt.Println(msg)
//
//case <-t1.C:
//println("5s timer")
//t1.Reset(time.Second * 5)
//
//case <-t2.C:
//println("10s timer")
//t2.Reset(time.Second * 10)
//}
//}

//
//func sum(s []int, c chan int) {
//	sum := 0
//	for _, v := range s {
//		sum += v
//		time.Sleep(time.Second)
//	}
//	c <- sum // send sum to c
//}
//
//func main() {
//	s := []int{7, 2, 8, -9, 4, 0}
//	c := make(chan int)
//	go sum(s[:len(s)/2], c)
//	go sum(s[len(s)/2:], c)
//	x, y := <-c, <-c // receive from c
//	fmt.Println(x, y, x+y)
//}

/*
func test1() {
	var wg sync.WaitGroup
	wg.Add(1000)
	ticker := time.NewTicker(time.Microsecond * 1)
	lastTime := time.Now()
	go func() {
		for i := 0; i < 1000; i++ {
			<- ticker.C
			wg.Done()
		}
	}()
	wg.Wait()
	fmt.Println("done. secs = ", time.Now().Sub(lastTime).Seconds())
}
*/
