package layers

import "github.com/Frobeniusnorm/Flint/go/flint"

type ReLU struct {
	BaseLayer
}

func NewRelu() ReLU {
	return ReLU{
		BaseLayer: BaseLayer{
			trainable:  false,
			EnableGrad: false,
		},
	}
}

func (relu ReLU) Forward(x Tensor) Tensor {
	res := flint.Max(x.Node, int32(0))
	return Tensor{Node: res}
}

func (relu ReLU) String() string {
	return "ReLU()"
}
