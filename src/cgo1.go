package main
/*
unsigned long long D2ULL1(double d) {
	return *(unsigned long long*)(&d);
}
*/
import "C"
import (
	"fmt"
	"unsafe"
)

func TestD2ULL1(n int) {
	var d = float64(1.23456)
	var o = uint64(0)
	for i := 0; i < n; i++ {
		o = uint64(C.D2ULL1(C.double(d)))
	}
	fmt.Println(o)
}

func D2ULL2(d float64) uint64 {
	return *(*uint64)(unsafe.Pointer(&d))
}
func TestD2ULL2(n int) {
	var d = float64(1.23456)
	var o = uint64(0)
	for i := 0; i < n; i++ {
		o = D2ULL2(d)
	}
	fmt.Println(o)
}

func main() {
}
