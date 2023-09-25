#include "../layers.hpp"
#include <flint/flint.h>
#include <array>
#include <initializer_list>
/**
 * Padding of convolution operations
 * - `NO_PADDING`: a normal convolution operation. Each filter is slid over the
 *   input tensor with its step size as many times as it completly fits into the
 *   input tensor. The output may have a smaller size then the input.
 */
enum PaddingMode { NO_PADDING };
/**
 * A generic Convolution layer. It creates multiple filters that are slid along
 * the input in each dimension by a step size. Each time the filter values are
 * multiplied with the elements of the input tensor with which it is currently
 * aligned and the result (with shape of the filter) is summed up to a single
 * value in the resulting tensor. After that the filter is moved by its step
 * size in each dimension and the process repeats. After the convolution is
 * calculated a learnable bias is added to the result per filter.
 *
 * TLDR; Each element in the result of this layer is a full multiplication of a
 * filter with a corresponding window in the input array. This is especially
 * helpful for image processing tasks, since the parameters (filters) allow the
 * recognize location independent features in the input.
 *
 * You are supposed to configure this layer by providing a number of `filters`,
 * a `kernel_size` and the size of the last dimension of the input tensor, i.e.
 * the channels of the input tensor called `units_in`. The template expects you
 * to provide the dimensionality of the input tensor (including batch size and
 * channels). The output size is the same as of the input tensor for the first
 * dimension (usually the `batch_size`), in the last dimension it is the number
 * of `filters` and in every other the number of times each filter can be slid
 * against the input tensor (depending on the size of the input tensor, the
 * `kernel_size`, the step size and padding see `PaddingMode`).
 *
 * E.g. if you have a batch of two dimensional rgb (3 channels) images, it
 * would have a shape of `(batch_size, height, width, 3)`. Then you would
 * create a `Convolution<4>` layer (also called `Conv2D`) with `units_in = 3`.
 * The output tensor would also be a 4 dimensional tensor.
 * Lets say you dont use padding (`NO_PADDING`), 10 filters, a step size of 2 in
 * each dimension, 32 as `kernel_size` and your 100 images have widths and
 * heights of `128` (`input_shape = (100, 128, 128, 3)`).
 * The output size would be
 * `(batch_size, ceil((input_shape - kernel_size + 1) / steps),
 *   ceil((input_shape - kernel_size + 1) / steps), filters) =
 *   (100, 49, 49, 10)`.
 */
template <int n> class Convolution : public Layer<n, 1> {
  constexpr std::array<size_t, n> weight_shape(unsigned int filters,
                                               unsigned int kernel_size,
                                               size_t units_in) {
    std::array<size_t, n> res;
    res[0] = filters;
    for (int i = 1; i < n - 1; i++) {
      res[i] = kernel_size;
    }
    res[n - 1] = units_in;
    return res;
  }
  std::array<unsigned int, n - 1> act_stride;
  unsigned int kernel_size;
  void initialize_precalc(std::array<unsigned int, n - 2> stride) {
    act_stride[0] = 1;
    for (int i = 0; i < n - 2; i++)
      act_stride[i + 1] = stride[i];
  }

public:
  //static constexpr FType transform_type(FType t) { return F_FLOAT64; }
  PaddingMode padding_mode;
  /** Initializes the Convolution Layer.
   * - `units_in` number of channels (size of last dimension) of input tensor
   * - `filters` number of used filters (size of last dimension of the result
   *    tensor)
   * - `kernel_size` size of filters
   * - `weight_init` Initializer for filters, has to implement the `Initializer`
   *    concept, should generate random values close to a normal distribution
   * - `bias_init` Initializer for the bias, has to implement the `Initializer`
   *    concept, should generate small values, constant values like `0` are fine
   * - `stride` step size per dimension (2 dimensions less then the input
   *    tensor, since the convolution is broadcasted along the `batch_size` and
   *    the channels in the last dimension are fully reduced)
   * - `padding_mode` which type of padding to use (see `PaddingMode` for more
   *    information)
   */
  template <Initializer InitWeights, Initializer InitBias>
  Convolution(size_t units_in, unsigned int filters, unsigned int kernel_size,
              InitWeights weight_init, InitBias bias_init,
              std::array<unsigned int, n - 2> stride,
              PaddingMode padding_mode = NO_PADDING)
      : Layer<n, 1>(weight_init.template initialize<double>(
                        weight_shape(filters, kernel_size, units_in)),
                    bias_init.template initialize<double>(
                        std::array<size_t, 1>{(size_t)filters})),
        padding_mode(padding_mode), kernel_size(kernel_size) {
    initialize_precalc(stride);
  }

  /** Initializes the Convolution Layer.
   * - `units_in` number of channels (size of last dimension) of input tensor
   * - `filters` number of used filters (size of last dimension of the result
   *    tensor)
   * - `kernel_size` size of filters
   * - `stride` step size per dimension (2 dimensions less then the input
   *    tensor, since the convolution is broadcasted along the `batch_size` and
   *    the channels in the last dimension are fully reduced)
   * - `padding_mode` which type of padding to use (see `PaddingMode` for more
   *    information)
   *
   * The filters are initialized with a glorot uniform distribution.
   */
  Convolution(size_t units_in, unsigned int filters, unsigned int kernel_size,
              std::array<unsigned int, n - 2> stride,
              PaddingMode padding_mode = NO_PADDING)
      : Layer<n, 1>(GlorotUniform().template initialize<double>(
                        weight_shape(filters, kernel_size, units_in)),
                    ConstantInitializer().template initialize<double>(
                        std::array<size_t, 1>{(size_t)filters})),
        padding_mode(padding_mode), kernel_size(kernel_size) {

    initialize_precalc(stride);
  }
  std::string name() override { return "Convolution"; }
  std::string summary() override {
    const unsigned int filters =
        Layer<n, 1>::template get_weight<0>().get_shape()[0];
    const unsigned int units_in =
        Layer<n, 1>::template get_weight<0>().get_shape()[n - 1];
    const unsigned int kernel_size =
        Layer<n, 1>::template get_weight<0>().get_shape()[1];
    return name() + ": input channels: " + std::to_string(units_in) +
           " filters: " + std::to_string(filters) +
           ", kernel size: " + std::to_string(kernel_size);
  }
  template <typename T, unsigned int k>
  Tensor<double, k> forward(Tensor<T, k> &in) {
    const unsigned int filters =
        Layer<n, 1>::template get_weight<0>().get_shape()[0];
    // actual convolve
    Tensor<double, k> res;
    for (unsigned int i = 0; i < filters; i++) {
      // in has shape [batch, dim1, ..., units_in]
      // has shape [1, kernel_size, ..., units_in]
      Tensor<double, n> filter =
          Layer<n, 1>::template get_weight<0>().slice(TensorRange(i, i + 1));
      Tensor<double, n - 1> filter_res = in.convolve_array(filter, act_stride);
      std::array<size_t, n> new_shape;
      for (int i = 0; i < n - 1; i++)
        new_shape[i] = filter_res.get_shape()[i];
      new_shape[n - 1] = 1;
      Tensor<double, k> local_res = filter_res.reshape_array(new_shape);
      local_res.execute();
      res = i == 0 ? local_res : Flint::concat(res, local_res, n - 1);
    }
    // repeat bias to the shape of res and add
    std::array<size_t, n - 1> bias_shape;
    bias_shape[n - 2] = filters;
    for (int i = 0; i < n - 2; i++)
      bias_shape[i] = 1;
    Tensor<double, n - 1> bias =
        Layer<n, 1>::template get_weight<1>().reshape_array(bias_shape);
    std::array<int, n - 1> bias_repeat;
    bias_repeat[n - 2] = 0;
    for (int i = 0; i < n - 2; i++)
      bias_repeat[i] = res.get_shape()[i + 1] - 1;
    bias = bias.repeat_array(bias_repeat);
    return res + bias;
  }
};
/** For inputs of images with shape `(batch_size, width, height, channels)` */
typedef Convolution<4> Conv2D;

enum PoolingMode { MAX_POOLING, MIN_POOLING, AVG_POOLING };
template <int n> class Pooling : public UntrainableLayer {
  std::array<size_t, n> window_size;
  std::array<unsigned int, n> step_size;
  PoolingMode mode;

  static Pooling<n> pooling_helper(PoolingMode mode, std::initializer_list<size_t> window_size, std::initializer_list<unsigned int> step_size) {
    std::array<size_t, n - 1> window_size_a;
    std::array<unsigned int, n - 1> step_size_a;
    window_size_a.fill(1);
    step_size_a.fill(1);
    int index = 0;
    for(size_t w : window_size) 
      window_size_a[index++] = w;
    index = 0;
    for(unsigned int s : step_size) 
      step_size_a[index++] = s;
    return Pooling(mode, window_size_a, step_size_a);
  }
public:
  Pooling(PoolingMode mode, std::array<size_t, n - 1> ws,
          std::array<unsigned int, n - 1> ss)
      : mode(mode) {
    std::memcpy(window_size.data() + 1, ws.data(), (n - 1) * sizeof(size_t));
    std::memcpy(step_size.data() + 1, ss.data(), (n - 1) * sizeof(unsigned int));
  }

  template <typename T, unsigned int k>
  Tensor<T, k> forward(Tensor<T, k> &in) {
    window_size[0] = in.get_shape()[0];
    step_size[0] = in.get_shape()[0];
    Tensor<T, n + 1> windows = in.sliding_window(window_size, step_size);
    std::array<size_t, n> final_shape;
    std::array<int, n + 1> transpose;
    final_shape[0] = in.get_shape()[0];
    for (int i = n; i >= 2; i--) {
      Tensor<T, n> red;
      switch (mode) {
        case MAX_POOLING:
          red = windows.reduce_max(i); 
        break;
        case MIN_POOLING:
          red = windows.reduce_min(i); 
        break;
        case AVG_POOLING:
          red = windows.reduce_sum(i) / (long)windows.get_shape()[i]; 
        break;
      }
      windows = red.expand(i, 1);
      size_t win = in.get_shape()[i - 1] - window_size[i - 1] + 1;
      win = win % step_size[i - 1] == 0 ? win / step_size[i - 1]
                                    : win / step_size[i - 1] + 1;
      final_shape[i - 1] = win;
      transpose[i] = i;
    }
    transpose[0] = 1;
    transpose[1] = 0;
    windows = windows.transpose_array(transpose);
    // transpose so memory order is correct
    return windows.reshape_array(final_shape);
  }

  std::string name() override { 
    std::string method;
    switch(mode) {
      case MAX_POOLING: method = "Max"; break;
      case MIN_POOLING: method = "Min"; break;
      case AVG_POOLING: method = "Avg"; break;
    }
    return method + "Pooling"; 
  }
  static Pooling<n> max_pooling(std::initializer_list<size_t> window_size, std::initializer_list<unsigned int> step_size) {
    return pooling_helper(MAX_POOLING, window_size, step_size);
  }
  static Pooling<n> min_pooling(std::initializer_list<size_t> window_size, std::initializer_list<unsigned int> step_size) {
    return pooling_helper(MIN_POOLING, window_size, step_size);
  }
  static Pooling<n> avg_pooling(std::initializer_list<size_t> window_size, std::initializer_list<unsigned int> step_size) {
    return pooling_helper(AVG_POOLING, window_size, step_size);
  }
};
