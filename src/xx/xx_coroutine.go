package xx

import (
	"runtime"
	"time"
)

// 以单线程方式执行一组协程


type Coroutine struct {
	cors []*Cor
	C chan int
}

func (zs *Coroutine) New() (c *Cor) {
	c = &Cor{ zs, make(chan int), len(zs.cors) }
	zs.cors = append(zs.cors, c)
	return
}

func (zs *Coroutine) Delete(c *Cor) {
	idx := c.Index
	count := len(zs.cors)
	if idx + 1 < count {
		zs.cors[count - 1].Index = idx
		zs.cors[idx] = zs.cors[count - 1]
	}
	zs.cors = zs.cors[:count - 1]
}

func (zs *Coroutine) Resume() bool {
	if len(zs.cors) == 0 {
		return true
	}
	for i := len(zs.cors) - 1; i >= 0; i-- {
		c := zs.cors[i]
		c.C <- 1
		if <- zs.C == 0 {
			zs.Delete(c)
		}
	}
	return false
}

type Cor struct {
	Ctx *Coroutine
	C chan int
	Index int
}

func (zs *Cor) Begin() {
	<- zs.C
}

func (zs *Cor) Yield() {
	zs.Ctx.C <- 1
	if 0 == <- zs.C {
		runtime.Goexit()
	}
}

func (zs *Cor) End() {
	zs.Ctx.C <- 0
	runtime.Goexit()
}

func (zs *Cor) Sleep(frames int) {
	for i := 0; i < frames; i++ {
		zs.Yield()
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
				return
			}
			goto Begin
		} else {
			time.Sleep(durationPerFrame - durationPool)
		}
	}
}

func NewCoroutine() *Coroutine {
	zs := &Coroutine{}
	zs.C = make(chan int)
	return zs
}



/*

// 多线程版

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


func Test(cor *xx.Coroutine, prefix string) {
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
	go Test(cor, "cor1")
	go Test(cor, "cor2")
	go Test(cor, "cor3")
	cor.Run(time.Second / 2)
}
*/
