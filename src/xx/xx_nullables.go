package xx

type NullableUInt8 struct {
	Value uint8
	HasValue bool
}
type NullableUInt16 struct {
	Value uint16
	HasValue bool
}
type NullableUInt32 struct {
	Value uint32
	HasValue bool
}
type NullableUInt64 struct {
	Value uint64
	HasValue bool
}
type NullableInt8 struct {
	Value int8
	HasValue bool
}
type NullableInt16 struct {
	Value int16
	HasValue bool
}
type NullableInt32 struct {
	Value int32
	HasValue bool
}
type NullableInt64 struct {
	Value int64
	HasValue bool
}
type NullableBool struct {
	Value bool
	HasValue bool
}
type NullableFloat32 struct {
	Value float32
	HasValue bool
}
type NullableFloat64 struct {
	Value float64
	HasValue bool
}
type NullableString struct {
	Value string
	HasValue bool
}

// Serial helper
type String string
func (zs *String) GetPackageId() uint16 {
	return 1
}
func (zs *String) ToBBuffer(bb *BBuffer) {
	bb.WriteLength(len(*zs))
	bb.Buf = append(bb.Buf, *zs...)
}
func (zs *String) FromBBuffer(bb *BBuffer) {
	bufLen := bb.ReadLength()
	*zs = String(bb.Buf[bb.Offset:bb.Offset + bufLen])
	bb.Offset += bufLen
}
