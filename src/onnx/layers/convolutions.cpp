#include "flint.h"
#include "../layers.hpp"
#include <iostream>
void Convolve::forward() {
#ifdef FLINT_DEBUG
	if ((incoming.size() != 2 && incoming.size() != 3) ||
		incoming[0]->output.size() != 1 || incoming[1]->output.size() != 1 ||
		(incoming.size() == 3 && incoming[2]->output.size() != 1))
		flogging(F_ERROR, "Convolve expects an image and a kernel as "
						  "parameters and optionally a bias");
#endif
	FGraphNode *weight = incoming[1]->output[0];
	FGraphNode *image = incoming[0]->output[0];
  int transpositions [image->operation.dimensions];
  for (int i = 0; i < image->operation.dimensions; i++)
    transpositions[i] = i;
  transpositions[1] = image->operation.dimensions - 1;
  transpositions[image->operation.dimensions - 1] = 1;
  image = ftranspose(image, transpositions);
  weight = ftranspose(weight, transpositions);
	FGraphNode *bias = incoming.size() == 3 ? incoming[2]->output[0] : nullptr;
	// expand kernel s.t. it matches the batch size
	FGraphNode *eweight = fexpand(weight, 1, 1);
	using namespace std;
	vector<unsigned int> steps(stride.size() + 1);
	steps[0] = 1;
	for (int i = 1; i < steps.size(); i++)
		steps[i] = stride[i - 1];
	// adapt image with padding
	vector<size_t> padded_shape(image->operation.dimensions);
	vector<size_t> inclusion_index(image->operation.dimensions, 0);
	for (int i = 0; i < padded_shape.size(); i++) {
		padded_shape[i] = image->operation.shape[i];
		if (padding.size() != 0 && i > 0 && i < padded_shape.size() - 1) {
			inclusion_index[i] = padding[i - 1];
			padded_shape[i] += padding[i - 1] +
							  padding[i - 1 + image->operation.dimensions - 2];
		}
	}
	image = fextend(image, padded_shape.data(), inclusion_index.data());
	// do the convolution
	output[0] = fconvolve(image, eweight, steps.data());
	if (bias)
		output[0] = fadd(output[0], bias);
}

void MaxPool::forward() {
#ifdef FLINT_DEBUG
	if (incoming.size() != 1 || incoming[0]->output.size() != 1)
		flogging(F_ERROR, "MaxPool expects an image as inputs");
#endif
	using namespace std;
	vector<unsigned int> steps(stride.size() + 1);
	steps[0] = 1;
	for (int i = 1; i < steps.size(); i++)
		steps[i] = stride[i - 1];
	// adapt image with padding
	FGraphNode *image = incoming[0]->output[0];
	vector<size_t> padded_shape(image->operation.dimensions);
	vector<size_t> inclusion_index(image->operation.dimensions, 0);
	for (int i = 0; i < padded_shape.size(); i++) {
		padded_shape[i] = image->operation.shape[i];
		if (padding.size() != 0 && i > 0 && i < padded_shape.size() - 1) {
			inclusion_index[i] = padding[i - 1];
			padded_shape[i] += padding[i - 1] +
							  padding[i - 1 + image->operation.dimensions - 2];
		}
	}
	image = fextend(image, padded_shape.data(), inclusion_index.data());
  size_t windows[kernel_shape.size() + 1];
  for (int i = 1; i < kernel_shape.size() + 1; i++)
    windows[i] = kernel_shape[i - 1];
  windows[0] = 1;
	// do the pooling
	output[0] = fexpand(fpooling_max(image, windows, steps.data()), image->operation.dimensions - 1, 1);
}
void AvgPool::forward() {
#ifdef FLINT_DEBUG
	if (incoming.size() != 1 || incoming[0]->output.size() != 1)
		flogging(F_ERROR, "AvgPool expects an image as inputs");
#endif
	using namespace std;
	vector<unsigned int> steps(stride.size() + 1);
	steps[0] = 1;
	for (int i = 1; i < steps.size(); i++)
		steps[i] = stride[i - 1];
	// adapt image with padding
	FGraphNode *image = incoming[0]->output[0];
	vector<size_t> padded_shape(image->operation.dimensions);
	vector<size_t> inclusion_index(image->operation.dimensions, 0);
	for (int i = 0; i < padded_shape.size(); i++) {
		padded_shape[i] = image->operation.shape[i];
		if (padding.size() != 0 && i > 0 && i < padded_shape.size() - 1) {
			inclusion_index[i] = padding[i - 1];
			padded_shape[i] = padding[i - 1] +
							  padding[i - 1 + image->operation.dimensions - 2];
		}
	}
	image = fextend(image, padded_shape.data(), inclusion_index.data());
  size_t windows[kernel_shape.size() + 1];
  size_t window_total = 1;
  for (int i = 1; i < kernel_shape.size() + 1; i++) {
    windows[i] = kernel_shape[i - 1];
    window_total *= windows[i];
  }
  windows[0] = 1;
	// do the pooling
	output[0] = fexpand(fdiv_ci(fpooling_sum(image, windows, steps.data()), window_total), image->operation.dimensions - 1, 1);
}
void GlobalAvgPool::forward() {
#ifdef FLINT_DEBUG
	if (incoming.size() != 1 || incoming[0]->output.size() != 1)
		flogging(F_ERROR, "AvgPool expects an image as inputs");
#endif
	using namespace std;
	FGraphNode *image = incoming[0]->output[0];
  // only first and last dimension are kept
  const int dims = image->operation.dimensions;
  while (image->operation.dimensions > 2) {
    image = fdiv_ci(freduce_sum(image, 2), image->operation.shape[2]);
  }
  // expand to fit original rank
  vector<size_t> rank_shape(dims, 1);
  rank_shape[0] = image->operation.shape[0];
  rank_shape[1] = image->operation.shape[1];
  output[0] = freshape(image, rank_shape.data(), dims);
}

