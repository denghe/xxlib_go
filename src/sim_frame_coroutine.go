package main

import (
	"./xx"
	"fmt"
	"time"
)

func Test(cor *xx.Cor, prefix string) {
	cor.Begin()
	n := 0
	for {
		fmt.Println(prefix, n)
		n++
		if n == 5 {
			break
		}
		cor.Yield()
	}
	cor.End()
}
func main() {
	cor := xx.NewCoroutine()
	go Test(cor.New(), "cor1")
	go Test(cor.New(), "cor2")
	go Test(cor.New(), "cor3")
	cor.Run(time.Second / 2)
}
