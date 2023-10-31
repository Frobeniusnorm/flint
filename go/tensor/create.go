package tensor

import (
	"github.com/Frobeniusnorm/Flint/go/flint"
)

// flatten is a recursive function to flatten a slice of any type
func flattenSlice(x any) (result []any, shape Shape, err error) {
	//shape, err = sliceShape(x)
	//if err != nil {
	//	return
	//}
	//
	//for _, item := range x {
	//	switch s := item.(type):
	//		case
	//
	//	result = append(result, s...)
	//}
	return nil, Shape{}, nil
}

func sliceShape(x any) (Shape, error) {
	return Shape{}, nil
}

// isTensor returns whether a given object is a slice or not
func isSlice(x any) bool {
	// TODO: check if reflect.kind matches slice
	return false
}

// isScalar returns whether given object is a scalar (numeric) value or not
func isScalar(x any) bool {
	// TODO: check if kind matches numeric
	//return reflect.
	return false
}

/*
Create a Tensor from a wide range of data types.

- if scalar type just keep it, don't turn into tensor yet!
- if nd array flatten it a generate
- if 1d array generate normally
- if [flint.GraphNode], just set reference counter

TODO: what if a pointer is passed?
*/
func Create[T Numeric](data any) Tensor {
	if isSlice(data) {
		return Tensor{} // CreateFromSlice(data)
	}
	if isScalar(data) {
		return Tensor{} // Scalar(data)
	}
	if node, ok := data.(flint.GraphNode); ok {
		return FromNode[T](node)
	}
	panic("invalid data type")
}

//
//func CreateFromSlice(data any) Tensor {
//	x := flatten(data)
//	shape := Shape{1, 2} // TODO
//
//	flintNode, err := flint.CreateGraph(flatten(x), shape)
//	if err != nil {
//		panic(err)
//	}
//	tensor.node = &flintNode
//
//	tensor.init()
//	return tensor
//}

//func CreateFromSliceAndShape[T Numeric](data []T, shape Shape) Tensor {
//	x := flatten(data)
//	tensor := Tensor{}
//	flintNode, err := flint.CreateGraph(flatten(x), shape)
//	if err != nil {
//		panic(err)
//	}
//	tensor.node = &flintNode
//
//	tensor.init()
//	return tensor
//}

func FromNode[T Numeric](node flint.GraphNode) Tensor {
	res := Tensor{node: &node}
	res.init()
	return res
}

func Scalar[T Numeric](val T) Tensor {
	tensor := Tensor{data: &val}
	tensor.init()
	return tensor
}

func Constant[T Numeric](val T, shape Shape) Tensor {
	flintNode, err := flint.CreateGraphConstant(val, shape)
	if err != nil {
		panic(err)
	}
	tensor := Tensor{node: &flintNode}
	tensor.init()
	return tensor
}

func Random(shape Shape) Tensor[float64] {
	flintNode, err := flint.CreateGraphRandom(shape)
	if err != nil {
		panic(err)
	}
	tensor := Tensor[float64]{node: &flintNode}
	tensor.init()
	return tensor
}

func Arrange(shape Shape, axis int) Tensor[int64] {
	flintNode, err := flint.CreateGraphArrange(shape, axis)
	if err != nil {
		panic(err)
	}
	tensor := Tensor[int64]{node: &flintNode}
	tensor.init()
	return tensor
}

func Identity[T Numeric](size T) Tensor[int32] {
	flintNode, err := flint.CreateGraphIdentity(uint(size))
	if err != nil {
		panic(err)
	}
	tensor := Tensor[int32]{node: &flintNode}
	tensor.init()
	return tensor
}
