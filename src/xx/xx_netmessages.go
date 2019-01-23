package xx

import "sync"

type NetMessage struct {
	TypeId int32									// 0: package     	1: request		2: response
	Serial int32									// needed by request or response
	Pkg IObject
}



type NetMessages struct {
	sync.Mutex
	recvs []*NetMessage
}
func NewNetMessages(capacity int) *NetMessages {
	ms := &NetMessages{}
	ms.recvs = make([]*NetMessage, 0, capacity)
	return ms
}
func (zs *NetMessages) Push(m *NetMessage) {
	zs.Lock()
	defer zs.Unlock()
	zs.recvs = append(zs.recvs, m)
}
func (zs *NetMessages) Pushs(ms []*NetMessage) {
	zs.Lock()
	defer zs.Unlock()
	zs.recvs = append(zs.recvs, ms...)
}
func (zs *NetMessages) Pop() (r *NetMessage) {
	zs.Lock()
	defer zs.Unlock()
	count := len(zs.recvs)
	if count == 0 {
		return
	}
	r = zs.recvs[0]
	if count == 1 {
		zs.recvs = zs.recvs[:0]
	} else {
		zs.recvs = zs.recvs[1:]
	}
	return
}
func (zs *NetMessages) Pops() (r []*NetMessage) {
	zs.Lock()
	defer zs.Unlock()
	if len(zs.recvs) == 0 {
		return
	}
	r = zs.recvs
	zs.recvs = make([]*NetMessage, 0, cap(r))
	return
}
func (zs *NetMessages) Clear() {
	zs.Lock()
	defer zs.Unlock()
	zs.recvs = zs.recvs[:0]
}
