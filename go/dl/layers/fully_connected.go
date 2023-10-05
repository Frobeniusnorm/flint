package layers

import (
	"fmt"
	"github.com/Frobeniusnorm/Flint/go/dl"
	"github.com/Frobeniusnorm/Flint/go/flint"
)

type FullyConnected struct {
	inputSize      uint
	outputSize     uint
	weightsAndBias dl.Parameter
}

// n = input size
// m = output_size

func NewFullyConnected(inputSize uint, outputSize uint) FullyConnected {
	weights := flint.CreateGraphRandom(flint.Shape{inputSize, outputSize})
	bias := flint.CreateGraphConstant(1, flint.Shape{1, outputSize}, flint.F_FLOAT32)
	weightsAndBias := dl.NewParameter(flint.Concat(weights, bias, 0))

	return FullyConnected{
		inputSize:      inputSize,
		outputSize:     outputSize,
		weightsAndBias: weightsAndBias,
	}
}

func (fc FullyConnected) Forward(x dl.Tensor) dl.Tensor {
	inputShape := x.Node.GetShape()
	inputShape[len(inputShape)-1] = 1
	ones := flint.CreateGraphConstant(1, inputShape, flint.F_INT32)
	combined := flint.Concat(x.Node, ones, uint(len(inputShape)-1))
	res := flint.Matmul(combined, fc.weightsAndBias.Node)
	return dl.NewTensor(res)
}

func (fc FullyConnected) TrainMode() {}

func (fc FullyConnected) EvalMode() {}

func (fc FullyConnected) Parameters(_ bool) []dl.Parameter {
	return []dl.Parameter{fc.weightsAndBias}
}

func (fc FullyConnected) String() string {
	return fmt.Sprintf("FullyConnected(in: %d, out: %d)", fc.inputSize, fc.outputSize)
}
