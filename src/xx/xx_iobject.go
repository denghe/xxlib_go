package xx

type IObject interface {
	GetPackageId() uint16
	ToBBuffer(bb *BBuffer)
	FromBBuffer(bb *BBuffer)
}

/**********************************************************************************************************************/
// typeIdCreatorMappings
/**********************************************************************************************************************/

var typeIdCreatorMappings = map[uint16] func() IObject {}

func RegisterInternals() {
	typeIdCreatorMappings[1] = func() IObject {
		s := String("")
		return &s
	}
	typeIdCreatorMappings[2] = func() IObject {
		return &BBuffer{}
	}
}

func Register(typeId uint16, maker func() IObject) {
	typeIdCreatorMappings[typeId] = maker
}

func CreateByTypeId(typeId uint16) IObject {
	maker, found := typeIdCreatorMappings[typeId]
	if found {
		return maker()
	}
	panic(-3)
}

