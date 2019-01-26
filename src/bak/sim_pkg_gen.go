package bak

import (
	"../xx"
	"fmt"
)

// 模拟生成物

type PKG_Foo struct {
	Id int32
	Name xx.NullableString
	Age xx.NullableInt32
	Parent PKG_Foo_Interface
}

// for base class convert simulate
type PKG_Foo_Interface interface {
	GetFoo() *PKG_Foo
}
func (zs *PKG_Foo) GetFoo() *PKG_Foo {
	return zs
}
func Write_PKG_Foo(zs *xx.BBuffer, v PKG_Foo_Interface) {
	if v == nil {
		zs.WriteIObject(nil)
	} else {
		zs.WriteIObject(v.(xx.IObject))
	}
}
func Read_PKG_Foo(zs *xx.BBuffer) PKG_Foo_Interface {
	v := zs.ReadIObject()
	if v == nil {
		return nil
	} else {
		return v.(PKG_Foo_Interface)
	}
}
func TryReadRoot_PKG_Foo(zs *xx.BBuffer) (r *PKG_Foo, ok bool) {
	defer func() {
		recover()
	}()
	v := zs.ReadRoot()
	if v != nil {
		r = v.(PKG_Foo_Interface).GetFoo()
	}
	ok = true
	return
}


// IObject implement
func (zs *PKG_Foo) GetPackageId() uint16 {
	return uint16(3)
}
func (zs *PKG_Foo) ToBBuffer(bb *xx.BBuffer) {
	bb.WriteInt32(zs.Id)
	bb.WriteNullableString(zs.Name)
	bb.WriteNullableInt32(zs.Age)
	Write_PKG_Foo(bb, zs.Parent)
}
func (zs *PKG_Foo) FromBBuffer(bb *xx.BBuffer) {
	zs.Id = bb.ReadInt32()
	zs.Name = bb.ReadNullableString()
	zs.Age = bb.ReadNullableInt32()
	zs.Parent = Read_PKG_Foo(bb)
}




type List_Int32 []int32

// for base class convert simulate
type List_Int32_Interface interface {
	GetList_Int32() *List_Int32
}
func (zs *List_Int32) GetList_Int32() *List_Int32 {
	return zs
}
func Write_List_Int32(zs *xx.BBuffer, v List_Int32_Interface) {
	if v == nil {
		zs.WriteIObject(nil)
	} else {
		zs.WriteIObject(v.(xx.IObject))
	}
}
func Read_List_Int32(zs *xx.BBuffer) List_Int32_Interface {
	v := zs.ReadIObject()
	if v == nil {
		return nil
	} else {
		return v.(List_Int32_Interface)
	}
}
func TryReadRoot_List_Int32(zs *xx.BBuffer) (r *List_Int32, ok bool) {
	defer func() {
		recover()
	}()
	v := zs.ReadRoot()
	if v != nil {
		r = v.(List_Int32_Interface).GetList_Int32()
	}
	ok = true
	return
}


// IObject implement
func (zs *List_Int32) GetPackageId() uint16 {
	return uint16(4)
}
func (zs *List_Int32) ToBBuffer(bb *xx.BBuffer) {
	bb.WriteLength(len(*zs))
	for _, v := range *zs {
		bb.WriteInt32(v)
	}
}
func (zs *List_Int32) FromBBuffer(bb *xx.BBuffer) {
	count := bb.ReadLength()
	if bb.ReadLengthLimit != 0 && count > bb.ReadLengthLimit {
		panic(-1)
	}
	*zs = (*zs)[:0]
	if count == 0 {
		return
	}
	for i := 0; i < count; i++ {
		*zs = append(*zs, bb.ReadInt32())
	}
}

// util func
func (zs *List_Int32) SwapRemoveAt(idx int) {
	count := len(*zs)
	if idx + 1 < count {
		(*zs)[idx] = (*zs)[count - 1]
	}
	*zs = (*zs)[:count - 1]
}






type PKG_FooExt1 struct {
	PKG_Foo
	ChildIds List_Int32_Interface
}

// for base class convert simulate
type PKG_FooExt1_Interface interface {
	GetFooExt1() *PKG_FooExt1
}
func (zs *PKG_FooExt1) GetFooExt1() *PKG_FooExt1 {
	return zs
}
func Write_PKG_FooExt1(zs *xx.BBuffer, v PKG_FooExt1_Interface) {
	if v == nil {
		zs.WriteIObject(nil)
	} else {
		zs.WriteIObject(v.(xx.IObject))
	}
}
func Read_PKG_FooExt1(zs *xx.BBuffer) PKG_FooExt1_Interface {
	v := zs.ReadIObject()
	if v == nil {
		return nil
	} else {
		return v.(PKG_FooExt1_Interface)
	}
}
func TryReadRoot_PKG_FooExt1(zs *xx.BBuffer) (r *PKG_FooExt1, ok bool) {
	defer func() {
		recover()
	}()
	v := zs.ReadRoot()
	if v != nil {
		r = v.(PKG_FooExt1_Interface).GetFooExt1()
	}
	ok = true
	return
}


// IObject implement
func (zs *PKG_FooExt1) GetPackageId() uint16 {
	return uint16(5)
}
func (zs *PKG_FooExt1) ToBBuffer(bb *xx.BBuffer) {
	zs.PKG_Foo.ToBBuffer(bb)
	Write_List_Int32(bb, zs.ChildIds)
}
func (zs *PKG_FooExt1) FromBBuffer(bb *xx.BBuffer) {
	zs.PKG_Foo.FromBBuffer(bb)
	zs.ChildIds = Read_List_Int32(bb)
}








type List_PKG_FooExt1 []PKG_FooExt1_Interface

// for base class convert simulate
type List_PKG_FooExt1_Interface interface {
	GetList_PKG_FooExt1() *List_PKG_FooExt1
}
func (zs *List_PKG_FooExt1) GetList_PKG_FooExt1() *List_PKG_FooExt1 {
	return zs
}

func Write_List_PKG_FooExt1(zs *xx.BBuffer, v List_PKG_FooExt1_Interface) {
	if v == nil {
		zs.WriteIObject(nil)
	} else {
		zs.WriteIObject(v.(xx.IObject))
	}
}
func Read_List_PKG_FooExt1(zs *xx.BBuffer) List_PKG_FooExt1_Interface {
	v := zs.ReadIObject()
	if v == nil {
		return nil
	} else {
		return v.(List_PKG_FooExt1_Interface)
	}
}
func TryReadRoot_List_PKG_FooExt1(zs *xx.BBuffer) (r *List_PKG_FooExt1, ok bool) {
	defer func() {
		recover()
	}()
	v := zs.ReadRoot()
	if v != nil {
		r = v.(List_PKG_FooExt1_Interface).GetList_PKG_FooExt1()
	}
	ok = true
	return
}


// IObject implement
func (zs *List_PKG_FooExt1) GetPackageId() uint16 {
	return uint16(6)
}
func (zs *List_PKG_FooExt1) ToBBuffer(bb *xx.BBuffer) {
	bb.WriteLength(len(*zs))
	for _, v := range *zs {
		Write_PKG_FooExt1(bb, v)
	}
}
func (zs *List_PKG_FooExt1) FromBBuffer(bb *xx.BBuffer) {
	count := bb.ReadLength()
	if bb.ReadLengthLimit != 0 && count > bb.ReadLengthLimit {
		panic(-1)
	}
	*zs = (*zs)[:0]
	if count == 0 {
		return
	}
	for i := 0; i < count; i++ {
		*zs = append(*zs, Read_PKG_FooExt1(bb))
	}
}

// util func
func (zs *List_PKG_FooExt1) SwapRemoveAt(idx int) {
	count := len(*zs)
	if idx + 1 < count {
		(*zs)[idx] = (*zs)[count - 1]
	}
	*zs = (*zs)[:count - 1]
}










func PKG_RegisterAll() {
	xx.RegisterInternals()
	xx.Register(3, func() xx.IObject {
		return &PKG_Foo{}
	})
	xx.Register(4, func() xx.IObject {
		return &List_Int32{}
	})
	xx.Register(5, func() xx.IObject {
		return &PKG_FooExt1{}
	})
	xx.Register(6, func() xx.IObject {
		return &List_PKG_FooExt1{}
	})
	// ... more
}


// todo: 测试枚举模拟. 参考 pb3, lua


func main() {
	PKG_RegisterAll()
	bb := &xx.BBuffer{}

	foo := &PKG_Foo{
		10,
		xx.NullableString{"asdf", true},
		xx.NullableInt32{0, false},
		nil,
	}
	//foo.Parent = foo
	bb.WriteRoot(foo)

	fmt.Println(foo)
	fmt.Println(bb)

	if foo2, ok := TryReadRoot_PKG_Foo(bb); ok {
		fmt.Println(foo2)
	}


	fooExt1 := &PKG_FooExt1{*foo, &List_Int32{ 1,2,3,4 }}
	fooExt1.Parent = fooExt1
	bb.Clear()
	bb.WriteRoot(fooExt1)

	fmt.Println(fooExt1)
	fmt.Println(bb)

	if fooExt2, ok := TryReadRoot_PKG_FooExt1(bb); ok {
		fmt.Println(fooExt2)
	}


	fooExts := &List_PKG_FooExt1{}
	*fooExts = append(*fooExts, fooExt1)
	*fooExts = append(*fooExts, fooExt1)
	*fooExts = append(*fooExts, fooExt1)

	bb.Clear()
	bb.WriteRoot(fooExts)
	fmt.Println(fooExts)
	fmt.Println(bb)

	if fooExts2, ok := TryReadRoot_List_PKG_FooExt1(bb); ok {
		fmt.Println(fooExts2)
		fmt.Println((*fooExts2)[0])
	} else {
		fmt.Println(ok)
	}
}
