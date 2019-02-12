package main
/*
void Add1(int* n) {
	++*n;
}
*/
import "C"
import "testing"

func main() {

}

func BenchmarkLoops(b *testing.B) {
	for i:=0; i< b.N; i++ {

	}
}
