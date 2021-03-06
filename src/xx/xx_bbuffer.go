package xx

import (
	"fmt"
	"reflect"
	"unsafe"
)

type BBuffer struct {
	Buf []uint8
	Offset,
	ReadLengthLimit,
	DataLenBak,
	OffsetRoot int
	strStore map[string] *String
	ptrStore map[IObject] int
	idxStore map[int] IObject
}

func NewBBuffer() *BBuffer {
	return &BBuffer{}
}

func (zs *BBuffer) DataLen() int {
	return len(zs.Buf)
}
func (zs *BBuffer) Clear() {
	zs.Buf = zs.Buf[:0]
	zs.Offset = 0
	zs.ReadLengthLimit = 0
}

func (zs *BBuffer) GetPackageId() uint16 {
	return 2
}
func (zs *BBuffer) ToBBuffer(bb *BBuffer) {
	bb.WriteLength(len(zs.Buf))
	bb.Buf = append(bb.Buf, zs.Buf...)
}
func (zs *BBuffer) FromBBuffer(bb *BBuffer) {
	bufLen := bb.ReadLength()
	if bb.ReadLengthLimit != 0 && bufLen > bb.ReadLengthLimit {
		panic(-1)
	}
	zs.Clear()
	zs.Buf = append(zs.Buf, bb.Buf[bb.Offset:bb.Offset + bufLen]...)
	bb.Offset += bufLen
}

// for all fix uint
func (zs *BBuffer) WriteVarUInt64(v uint64) {
	for v >= 1<<7 {
		zs.Buf = append(zs.Buf, uint8(v & 0x7f | 0x80))
		v >>= 7
	}
	zs.Buf = append(zs.Buf, uint8(v))
}
func (zs *BBuffer) ReadVarUInt64() (r uint64) {
	offset := zs.Offset
	for shift := uint(0); shift < 64; shift += 7 {
		b := uint64(zs.Buf[offset])
		offset++
		r |= (b & 0x7F) << shift
		if (b & 0x80) == 0 {
			zs.Offset = offset
			return r
		}
	}
	panic(-1)
}


// for all fix int
func (zs *BBuffer) WriteVarInt64(v int64) {
	zs.WriteVarUInt64( uint64(uint64(v << 1) ^ uint64(v >> 63)) )
}
func (zs *BBuffer) ReadVarInt64() int64 {
	v := zs.ReadVarUInt64()
	return (int64)(v >> 1) ^ (-(int64)(v & 1))
}

// for all len
func (zs *BBuffer) WriteLength(v int) {
	zs.WriteVarUInt64( uint64(v) )
}
func (zs *BBuffer) ReadLength() int {
	return int(zs.ReadVarUInt64())
}



func (zs *BBuffer) WriteFloat32(v float32) {
	f := *(*uint32)(unsafe.Pointer(&v))
	zs.Buf = append(zs.Buf,
		uint8(f),
		uint8(f >> 8),
		uint8(f >> 16),
		uint8(f >> 24))
}
func (zs *BBuffer) ReadFloat32() (r float32) {
	v := uint32(zs.Buf[zs.Offset]) |
		uint32(zs.Buf[zs.Offset + 1]) << 8 |
		uint32(zs.Buf[zs.Offset + 2]) << 16 |
		uint32(zs.Buf[zs.Offset + 3]) << 24
	r = *(*float32)(unsafe.Pointer(&v))
	zs.Offset += 4
	return
}

func (zs *BBuffer) WriteFloat64(v float64) {
	d := *(*uint64)(unsafe.Pointer(&v))
	zs.Buf = append(zs.Buf,
		uint8(d),
		uint8(d >> 8),
		uint8(d >> 16),
		uint8(d >> 24),
		uint8(d >> 32),
		uint8(d >> 40),
		uint8(d >> 48),
		uint8(d >> 56))
}
func (zs *BBuffer) ReadFloat64() (r float64) {
	v := uint32(zs.Buf[zs.Offset]) |
		uint32(zs.Buf[zs.Offset + 1]) << 8 |
		uint32(zs.Buf[zs.Offset + 2]) << 16 |
		uint32(zs.Buf[zs.Offset + 3]) << 24 |
		uint32(zs.Buf[zs.Offset + 4]) << 32 |
		uint32(zs.Buf[zs.Offset + 5]) << 40 |
		uint32(zs.Buf[zs.Offset + 6]) << 48 |
		uint32(zs.Buf[zs.Offset + 7]) << 56
	r = *(*float64)(unsafe.Pointer(&v))
	zs.Offset += 8
	return
}

func (zs *BBuffer) WriteBool(v bool) {
	if v {
		zs.Buf = append(zs.Buf, uint8(1))
	} else {
		zs.Buf = append(zs.Buf, uint8(0))
	}
}
func (zs *BBuffer) ReadBool() (r bool) {
	r = zs.Buf[zs.Offset] > 0
	zs.Offset++
	return
}

func (zs *BBuffer) WriteInt8(v int8) {
	zs.Buf = append(zs.Buf, uint8(v))
}
func (zs *BBuffer) ReadInt8() (r int8) {
	r = int8(zs.Buf[zs.Offset])
	zs.Offset++
	return
}

func (zs *BBuffer) WriteInt16(v int16) {
	zs.WriteVarInt64(int64(v))
}
func (zs *BBuffer) ReadInt16() int16 {
	return int16(zs.ReadVarInt64())
}

func (zs *BBuffer) WriteInt32(v int32) {
	zs.WriteVarInt64(int64(v))
}
func (zs *BBuffer) ReadInt32() int32 {
	return int32(zs.ReadVarInt64())
}

func (zs *BBuffer) WriteInt64(v int64) {
	zs.WriteVarInt64(v)
}
func (zs *BBuffer) ReadInt64() int64 {
	return zs.ReadVarInt64()
}

func (zs *BBuffer) WriteUInt8(v uint8) {
	zs.Buf = append(zs.Buf, v)
}
func (zs *BBuffer) ReadUInt8() (r uint8) {
	r = zs.Buf[zs.Offset]
	zs.Offset++
	return
}

func (zs *BBuffer) WriteUInt16(v uint16) {
	zs.WriteVarUInt64(uint64(v))
}
func (zs *BBuffer) ReadUInt16() uint16 {
	return uint16(zs.ReadVarUInt64())
}

func (zs *BBuffer) WriteUInt32(v uint32) {
	zs.WriteVarUInt64(uint64(v))
}
func (zs *BBuffer) ReadUInt32() uint32 {
	return uint32(zs.ReadVarUInt64())
}

func (zs *BBuffer) WriteUInt64(v uint64) {
	zs.WriteVarUInt64(v)
}
func (zs *BBuffer) ReadUInt64() uint64 {
	return zs.ReadVarUInt64()
}

func (zs *BBuffer) WriteNullableBool(v NullableBool) {
	if v.HasValue {
		zs.Buf = append(zs.Buf, uint8(1))
		zs.WriteBool(v.Value)
	} else {
		zs.Buf = append(zs.Buf, uint8(0))
	}
}
func (zs *BBuffer) ReadNullableBool() (r NullableBool) {
	r.HasValue = zs.ReadBool()
	if r.HasValue {
		r.Value = zs.ReadBool()
	}
	return
}
func (zs *BBuffer) WriteNullableFloat32(v NullableFloat32) {
	if v.HasValue {
		zs.Buf = append(zs.Buf, uint8(1))
		zs.WriteFloat32(v.Value)
	} else {
		zs.Buf = append(zs.Buf, uint8(0))
	}
}
func (zs *BBuffer) ReadNullableFloat32() (r NullableFloat32) {
	r.HasValue = zs.ReadBool()
	if r.HasValue {
		r.Value = zs.ReadFloat32()
	}
	return
}

func (zs *BBuffer) WriteNullableFloat64(v NullableFloat64) {
	if v.HasValue {
		zs.Buf = append(zs.Buf, uint8(1))
		zs.WriteFloat64(v.Value)
	} else {
		zs.Buf = append(zs.Buf, uint8(0))
	}
}
func (zs *BBuffer) ReadNullableFloat64() (r NullableFloat64) {
	r.HasValue = zs.ReadBool()
	if r.HasValue {
		r.Value = zs.ReadFloat64()
	}
	return
}

func (zs *BBuffer) WriteNullableString(v NullableString) {
	if v.HasValue {
		s, found := zs.strStore[v.Value]
		if !found {
			str := String(v.Value)
			s = &str
			zs.strStore[v.Value] = s
		}
		zs.WriteIObject(s)
	} else {
		zs.Buf = append(zs.Buf, uint8(0))
	}
}
func (zs *BBuffer) ReadNullableString() (r NullableString) {
	o := zs.ReadIObject()
	switch o.(type) {
	case nil:
	case *String:
		r.Value = *(*string)(o.(*String))
		r.HasValue = true
	default:
		panic(-1)
	}
	return
}

func (zs *BBuffer) WriteNullableUInt8(v NullableUInt8) {
	if v.HasValue {
		zs.Buf = append(zs.Buf, uint8(1), v.Value)
	} else {
		zs.Buf = append(zs.Buf, uint8(0))
	}
}
func (zs *BBuffer) ReadNullableUInt8() (r NullableUInt8) {
	r.HasValue = zs.ReadBool()
	if r.HasValue {
		r.Value = zs.ReadUInt8()
	}
	return
}

func (zs *BBuffer) WriteNullableUInt16(v NullableUInt16) {
	if v.HasValue {
		zs.Buf = append(zs.Buf, uint8(1))
		zs.WriteVarUInt64(uint64(v.Value))
	} else {
		zs.Buf = append(zs.Buf, uint8(0))
	}
}
func (zs *BBuffer) ReadNullableUInt16() (r NullableUInt16) {
	r.HasValue = zs.ReadBool()
	if r.HasValue {
		r.Value = zs.ReadUInt16()
	}
	return
}

func (zs *BBuffer) WriteNullableUInt32(v NullableUInt32) {
	if v.HasValue {
		zs.Buf = append(zs.Buf, uint8(1))
		zs.WriteVarUInt64(uint64(v.Value))
	} else {
		zs.Buf = append(zs.Buf, uint8(0))
	}
}
func (zs *BBuffer) ReadNullableUInt32() (r NullableUInt32) {
	r.HasValue = zs.ReadBool()
	if r.HasValue {
		r.Value = zs.ReadUInt32()
	}
	return
}

func (zs *BBuffer) WriteNullableUInt64(v NullableUInt64) {
	if v.HasValue {
		zs.Buf = append(zs.Buf, uint8(1))
		zs.WriteVarUInt64(v.Value)
	} else {
		zs.Buf = append(zs.Buf, uint8(0))
	}
}
func (zs *BBuffer) ReadNullableUInt64() (r NullableUInt64) {
	r.HasValue = zs.ReadBool()
	if r.HasValue {
		r.Value = zs.ReadUInt64()
	}
	return
}

func (zs *BBuffer) WriteNullableInt8(v NullableInt8) {
	if v.HasValue {
		zs.Buf = append(zs.Buf, uint8(1), uint8(v.Value))
	} else {
		zs.Buf = append(zs.Buf, uint8(0))
	}
}
func (zs *BBuffer) ReadNullableInt8() (r NullableInt8) {
	r.HasValue = zs.ReadBool()
	if r.HasValue {
		r.Value = zs.ReadInt8()
	}
	return
}

func (zs *BBuffer) WriteNullableInt16(v NullableInt16) {
	if v.HasValue {
		zs.Buf = append(zs.Buf, uint8(1))
		zs.WriteVarInt64(int64(v.Value))
	} else {
		zs.Buf = append(zs.Buf, uint8(0))
	}
}
func (zs *BBuffer) ReadNullableInt16() (r NullableInt16) {
	r.HasValue = zs.ReadBool()
	if r.HasValue {
		r.Value = zs.ReadInt16()
	}
	return
}

func (zs *BBuffer) WriteNullableInt32(v NullableInt32) {
	if v.HasValue {
		zs.Buf = append(zs.Buf, uint8(1))
		zs.WriteVarInt64(int64(v.Value))
	} else {
		zs.Buf = append(zs.Buf, uint8(0))
	}
}
func (zs *BBuffer) ReadNullableInt32() (r NullableInt32) {
	r.HasValue = zs.ReadBool()
	if r.HasValue {
		r.Value = zs.ReadInt32()
	}
	return
}

func (zs *BBuffer) WriteNullableInt64(v NullableInt64) {
	if v.HasValue {
		zs.Buf = append(zs.Buf, uint8(1))
		zs.WriteVarInt64(v.Value)
	} else {
		zs.Buf = append(zs.Buf, uint8(0))
	}
}
func (zs *BBuffer) ReadNullableInt64() (r NullableInt64) {
	r.HasValue = zs.ReadBool()
	if r.HasValue {
		r.Value = zs.ReadInt64()
	}
	return
}



func (zs *BBuffer) WriteSpaces(count int) {
	zs.Buf = append(zs.Buf, make([]uint8, count)...)
}






func (zs *BBuffer) BeginWrite() {
	zs.strStore = make(map[string] *String)
	zs.ptrStore = make(map[IObject] int)
	zs.OffsetRoot = len(zs.Buf)
}
func (zs *BBuffer) BeginRead() {
	zs.idxStore = make(map[int] IObject)
	zs.OffsetRoot = zs.Offset
}

func (zs *BBuffer) WriteRoot(v IObject) {
	zs.BeginWrite()
	zs.WriteIObject(v)
}
func (zs *BBuffer) ReadRoot() IObject {
	zs.BeginRead()
	return zs.ReadIObject()
}
func (zs *BBuffer) WriteIObject(v IObject) {
	//if *(*unsafe.Pointer)(unsafe.Pointer(uintptr(unsafe.Pointer(&v)) + uintptr(unsafe.Sizeof(int(0))))) == nil {
	if v == nil || reflect.ValueOf(v).IsNil() {
		zs.Buf = append(zs.Buf, uint8(0))
		return
	}
	zs.WriteUInt16(v.GetPackageId())
	offset, found := zs.ptrStore[v]
	if !found {
		offset = len(zs.Buf) - zs.OffsetRoot
		zs.ptrStore[v] = offset
	}
	zs.WriteLength(offset)
	if !found {
		v.ToBBuffer(zs)
	}
}
func (zs *BBuffer) ReadIObject() (r IObject) {
	typeId := zs.ReadUInt16()
	if typeId == 0 {
		return nil
	}
	offset := zs.Offset - zs.OffsetRoot
	ptrOffset := zs.ReadLength()
	if ptrOffset == offset {
		r = CreateByTypeId(typeId)
		zs.idxStore[ptrOffset] = r
		r.FromBBuffer(zs)
	} else {
		var found bool
		r, found = zs.idxStore[ptrOffset]
		if !found {
			panic(-5)
		}
		if r.GetPackageId() != typeId {
			panic(-6)
		}
	}
	return
}

func (zs *BBuffer) TryReadRoot() IObject {
	defer func() {
		if err := recover(); err != nil {
			fmt.Println("TryReadRoot error:", err)
		}
	}()
	zs.BeginRead()
	return zs.ReadIObject()
}
