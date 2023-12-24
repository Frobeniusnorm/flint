package losses

import (
	"github.com/Frobeniusnorm/Flint/go/flint"
	"github.com/Frobeniusnorm/Flint/go/wrapper"
	"github.com/stretchr/testify/assert"
	"testing"
)

func TestMSELoss(t *testing.T) {
	predictions := flint.NewTensor(wrapper.CreateScalar(5))
	target := flint.NewTensor(wrapper.CreateScalar(2))
	loss := MSELoss(predictions, target)
	res := wrapper.CalculateResult[int](loss.node).Data[0]

	assert.Equal(t, 9, res)
}
