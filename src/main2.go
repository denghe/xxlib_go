package main

import (
	"fmt"
	"sync/atomic"
	"time"
)

// 模拟 rpc

var rpcs = make(map[int32] chan interface{})
var rpcInc = int32(0)

func SendXxx(pkg interface{}, timeout time.Duration) (r interface{}) {
	serial := atomic.AddInt32(&rpcInc, int32(1))
	c := make(chan interface{})
	rpcs[serial] = c
	t := time.NewTimer(timeout)
	select {
		case <- t.C:
//			r = nil
		case v := <- c:
			r = v
	}
	return
}

func main() {
	rtv := SendXxx(nil, time.Second)
	fmt.Println(rtv)
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
