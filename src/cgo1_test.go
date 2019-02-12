package main
import "testing"

func BenchmarkLoops1(b *testing.B) {
	TestD2ULL1(b.N)
}
func BenchmarkLoops2(b *testing.B) {
	TestD2ULL2(b.N)
}
