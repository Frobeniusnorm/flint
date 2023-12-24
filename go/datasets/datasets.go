package datasets

import (
	"fmt"
	"github.com/Frobeniusnorm/Flint/go/flint"
	"github.com/Frobeniusnorm/Flint/go/wrapper"
	"log"
)

type baseDataset struct {
	Name string
}

type Dataset[T any] interface {
	fmt.Stringer
	Count() uint
	Get(index uint) (value T)

	// Collate combines multiple dataset entries into a single batched dataset entry
	Collate(items []T) T
}

const eps float32 = 1e-5

func TrainTestSplitAbsolute[T any](dataset Dataset[T], trainSize uint, testSize uint) (Dataset[T], Dataset[T]) {
	return dataset, dataset // TODO (implement. don't forget all the assertions about size)
}

func TrainTestSplitRelative[T any](dataset Dataset[T], trainPercentage float32, testPercentage float32) (Dataset[T], Dataset[T]) {
	return dataset, dataset // TODO (implement. don't forget all the assertions about size)
}

func TrivialCollate(items []flint.Tensor) flint.Tensor {
	if len(items) <= 0 {
		log.Panicf("cannot collate items - invalid batch size (%d)", len(items))
	}

	newShape := wrapper.Shape{1}
	for _, val := range items[0].Shape() {
		newShape = append(newShape, val)
	}

	var res = items[0].Reshape(newShape)
	items[0].Close()
	for _, val := range items[1:] {
		res = res.Concat(val.Reshape(newShape), 0)
		val.Close()
	}
	return res
}

// (debugging utility)
func printImage(image []int32, shape wrapper.Shape) {
	height, width := shape[0], shape[1]
	for row := uint(0); row < height; row++ {
		for col := uint(0); col < width; col++ {
			pix := image[row*height+col]
			if pix == 0 {
				fmt.Print(" ")
			} else {
				fmt.Printf("%X", pix/16)
			}
		}
		fmt.Println()
	}
}
