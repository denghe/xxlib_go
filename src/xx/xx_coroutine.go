package xx

import (
	"runtime"
	"sync"
	"time"
)

type Coroutine struct {
	mt sync.Mutex
	wg sync.WaitGroup
	chans []chan int
}
func NewCoroutine() *Coroutine {
	c := &Coroutine{}
	c.chans = make([]chan int, 0, 128)
	return c
}
func (zs *Coroutine) Begin() {
	zs.wg.Add(1)
}
func (zs *Coroutine) End() {
	zs.wg.Done()
}
func (zs *Coroutine) Yield() {
	c := make(chan int)
	func() {
		zs.mt.Lock()
		defer zs.mt.Unlock()
		zs.chans = append(zs.chans, c)
	}()
	zs.wg.Done()
	if 0 == <- c {
		runtime.Goexit()
	}
}
func (zs *Coroutine) Sleep(frames int) {
	for i := 0; i < frames; i++ {
		zs.Yield()
	}
}
func (zs *Coroutine) Resume() bool {
	zs.wg.Wait()
	zs.mt.Lock()
	defer zs.mt.Unlock()
	n := len(zs.chans)
	if n == 0 {
		return true
	}
	zs.wg.Add(n)
	for _, c := range zs.chans {
		c <- 1
	}
	zs.chans = zs.chans[:0]
	return false
}
func (zs *Coroutine) Cancel() {
	zs.wg.Wait()
	zs.mt.Lock()
	defer zs.mt.Unlock()
	for _, c := range zs.chans {
		close(c)
	}
}
// sample: time.Second / 60
func (zs *Coroutine) Run(durationPerFrame time.Duration) {
	if durationPerFrame <= 0 {
		for {
			if zs.Resume() {
				return
			}
		}
	}
	var durationPool time.Duration
	lastTime := time.Now()

	for {
		currTime := time.Now()
		durationPool += currTime.Sub(lastTime)
		lastTime = currTime

	Begin:
		if durationPool > durationPerFrame {
			durationPool -= durationPerFrame

			// todo: cancel cond check

			// todo: lag run check

			if zs.Resume() {
				break
			}
			goto Begin
		} else {
			time.Sleep(durationPerFrame - durationPool)
		}
	}
}
