/* Copyright 2023 David Schwarzbeck
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License. */
#include "shape_modification.hpp"
#include "../backend_ocl/utils.hpp"
#include "../utils.hpp"
#include "flint.h"
#include <cstring>

using namespace std;

FGraphNode *FlattenImpl::local_gradient(FGraphNode *y, int dx_i,
										FGraphNode *prev_adj) {
	FGraphNode *prev = y->predecessors[0];
	return freshape(prev_adj, prev->operation.shape,
					prev->operation.dimensions);
}
void FlattenImpl::execute_cpu(const FGraphNode *node,
							  vector<CPUResultData> predecessor_data,
							  void *__restrict__ result, size_t from,
							  size_t size) {
	const CPUResultData pred = predecessor_data[0];
	switch (predecessor_data[0].type) {
	case F_INT32:
	case F_FLOAT32:
		if (node->predecessors[0]->operation.op_type != FGEN_CONSTANT) {
			memcpy((int *)result + from, (int *)pred.data + from, 4 * size);
		} else {
			for (int i = from; i < from + size; i++)
				((int *)result)[i] = ((int *)pred.data)[0];
		}
		break;
	case F_INT64:
	case F_FLOAT64:
		if (node->predecessors[0]->operation.op_type != FGEN_CONSTANT) {
			memcpy((long *)result + from, (long *)pred.data + from, 8 * size);
		} else {
			for (int i = from; i < from + size; i++)
				((long *)result)[i] = ((long *)pred.data)[0];
		}
		break;
	}
}
int FlattenImpl::generate_ocl_lazy(const FGraphNode *node, string name,
								   OCLLazyCodegenState &compiler_state) {
	compiler_state.code.prepend(
		"const " + type_string(node->operation.data_type) + " " + name +
		" = v" + to_string(compiler_state.variable_index + 1) + ";\n");
	return 0;
}
FGraphNode *ConversionImpl::local_gradient(FGraphNode *y, int dx_i,
										   FGraphNode *prev_adj) {
	return prev_adj;
}
template <typename T, typename A>
void ConversionImpl::unary_expression(T *__restrict__ result,
									  const A *__restrict__ data1, size_t from,
									  size_t size, const FGraphNode *curr) {
	const FGraphNode *prev = curr->predecessors[0];
	size_t total_el_size = 1;
	if (prev->operation.op_type != FGEN_CONSTANT)
		for (int i = 0; i < prev->operation.dimensions; i++)
			total_el_size *= prev->operation.shape[i];
	for (size_t i = from; i < from + size; i++)
		result[i] = (T)data1[i % total_el_size];
}
int ConversionImpl::generate_ocl_lazy(const FGraphNode *node, string name,
									  OCLLazyCodegenState &compiler_state) {
	const string type = type_string(node->operation.data_type);
	compiler_state.code.prepend(
		"const " + type + " " + name + " = (" + type + ")v" +
		to_string(compiler_state.variable_index + 1) + ";\n");
	return 0;
}
void ConversionImpl::execute_cpu(const FGraphNode *node,
								 vector<CPUResultData> predecessor_data,
								 void *__restrict__ result, size_t from,
								 size_t size){UNARY_EXECUTE_IMPL}

FGraphNode *RepeatImpl::local_gradient(FGraphNode *y, int dx_i,
									   FGraphNode *prev_adj) {
	FGraphNode *a = y->predecessors[0];
	FGraphNode *grad = prev_adj;
	std::vector<size_t> orig_shape(prev_adj->operation.shape,
								   prev_adj->operation.shape +
									   prev_adj->operation.dimensions);
	for (int i = 0; i < a->operation.dimensions; i++) {
		if (a->operation.shape[i] != y->operation.shape[i]) {
			// reduce repeated gradient into correct shape
			std::vector<size_t> new_shape(orig_shape.size() + 1);
			// add extra dimension for reducing
			if (i > 0)
				std::memcpy(new_shape.data(), grad->operation.shape,
							sizeof(size_t) * i);
			new_shape[i] = orig_shape[i] / a->operation.shape[i];
			new_shape[i + 1] = a->operation.shape[i];
			if (orig_shape.size() - i > 1)
				std::memcpy(new_shape.data() + i + 2,
							grad->operation.shape + i + 1,
							sizeof(size_t) * (orig_shape.size() - i - 1));
			// reduce along that axis
			grad = freduce_sum(
				freshape(grad, new_shape.data(), new_shape.size()), i);
		}
	}
	return grad;
}
template <typename T>
void RepeatImpl::unary_expression(T *__restrict__ result,
								  const T *__restrict__ data, size_t from,
								  size_t size, const FGraphNode *curr) {
	const FOperation op = curr->operation;
	const FOperation pred = curr->predecessors[0]->operation;
	size_t total_el_size = 1;
	if (pred.op_type != FGEN_CONSTANT)
		for (int i = 0; i < pred.dimensions; i++)
			total_el_size *= pred.shape[i];
	// calculate number of elements per dimension entry for destination and
	// source
	vector<size_t> acc_sizes_d = calc_acc_sizes(op);
	vector<size_t> acc_sizes_s = calc_acc_sizes(pred.dimensions, pred.shape);
	for (int i = from; i < from + size; i++) {
		// to get the index in the source array we first calculate the
		// indices and reproject
		size_t index = i;
		size_t src_index = 0;
		for (int dim = 0; dim < op.dimensions; dim++) {
			int curr_idx = index / acc_sizes_d[dim];
			index %= acc_sizes_d[dim];
			src_index += (curr_idx % pred.shape[dim]) * acc_sizes_s[dim];
		}
		result[i] = data[src_index % total_el_size];
	}
}
int RepeatImpl::generate_ocl_lazy(const FGraphNode *node, string name,
								  OCLLazyCodegenState &compiler_state) {
	const FOperation op = node->operation;
	const FOperation pred = node->predecessors[0]->operation;
	const unsigned int old_idx = compiler_state.num_indices++;
	Twine index_defs;
	index_defs += "int old_index" + to_string(old_idx) + " = index;\n";
	// add to index_defs a redefinition of index, so that we remap
	// to src data calculate number of elements per dimension entry
	// for destination and source
	std::vector<size_t> acc_sizes_d(op.dimensions);
	std::vector<size_t> acc_sizes_s(op.dimensions);
	acc_sizes_d[op.dimensions - 1] = 1;
	acc_sizes_s[op.dimensions - 1] = 1;
	for (int dim = op.dimensions - 2; dim >= 0; dim--) {
		acc_sizes_d[dim] = acc_sizes_d[dim + 1] * op.shape[dim + 1];
		acc_sizes_s[dim] = acc_sizes_s[dim + 1] * pred.shape[dim + 1];
	}
	// to get the index in the source array we first calculate the
	// indices and reproject
	index_defs += "{\nint working_index = index;\nindex = 0;\n";
	for (int dim = 0; dim < op.dimensions; dim++) {
		index_defs += "index += ((working_index /" +
					  to_string(acc_sizes_d[dim]) + ") % " +
					  to_string(pred.shape[dim]) + ") * " +
					  to_string(acc_sizes_s[dim]) + ";\n";
		index_defs += "working_index %= " + to_string(acc_sizes_d[dim]) + ";\n";
	}
	index_defs += "}\n";
	compiler_state.index_defs = index_defs;
	compiler_state.code.prepend("index = old_index" + to_string(old_idx) +
								";\n");
	compiler_state.code.prepend(
		"const " + type_string(node->operation.data_type) + " " + name +
		" = v" + to_string(compiler_state.variable_index + 1) + ";\n");
	return 0;
}
void RepeatImpl::execute_cpu(const FGraphNode *node,
							 vector<CPUResultData> predecessor_data,
							 void *__restrict__ result, size_t from,
							 size_t size){UNARY_EXECUTE_MONOTON_IMPL}

FGraphNode *TransposeImpl::local_gradient(FGraphNode *y, int dx_i,
										  FGraphNode *prev_adj) {
	int *transp = ((int *)y->operation.additional_data);
	return ftranspose(prev_adj, transp);
}
template <typename T>
void TransposeImpl::unary_expression(T *__restrict__ result,
									 const T *__restrict__ data, size_t from,
									 size_t size, const FGraphNode *curr) {
	const FOperation op = curr->operation;
	const int *transposition = (int *)op.additional_data;
	const FOperation pred = curr->predecessors[0]->operation;
	size_t total_el_size = 1;
	if (pred.op_type != FGEN_CONSTANT)
		for (int i = 0; i < pred.dimensions; i++)
			total_el_size *= pred.shape[i];
	// calculate number of elements per dimension entry for destination and
	// source
	vector<size_t> acc_sizes_d = calc_acc_sizes(op);
	vector<size_t> acc_sizes_s = calc_acc_sizes(pred.dimensions, pred.shape);
	for (int i = from; i < from + size; i++) {
		// to get the index in the source array we first calculate the
		// indices and reproject
		int index = i;
		int src_index = 0;
		for (int dim = 0; dim < op.dimensions; dim++) {
			int curr_idx = index / acc_sizes_d[dim];
			index %= acc_sizes_d[dim];
			src_index += curr_idx * acc_sizes_s[transposition[dim]];
		}
		result[i] = data[src_index % total_el_size];
	}
}
int TransposeImpl::generate_ocl_lazy(const FGraphNode *node, string name,
									 OCLLazyCodegenState &compiler_state) {
	const FOperation op = node->operation;
	const int *transposition = (int *)op.additional_data;
	const FOperation pred = node->predecessors[0]->operation;
	unsigned int old_idx = compiler_state.num_indices++;
	Twine index_defs;
	index_defs += "long old_index" + to_string(old_idx) + " = index;\n";
	// add to index_defs a redefinition of index, so that we remap
	// to src data calculate number of elements per dimension entry
	// for destination and source
	std::vector<size_t> acc_sizes_d(op.dimensions);
	std::vector<size_t> acc_sizes_s(op.dimensions);
	acc_sizes_d[op.dimensions - 1] = 1;
	acc_sizes_s[op.dimensions - 1] = 1;
	for (int dim = op.dimensions - 2; dim >= 0; dim--) {
		acc_sizes_d[dim] = acc_sizes_d[dim + 1] * op.shape[dim + 1];
		acc_sizes_s[dim] = acc_sizes_s[dim + 1] * pred.shape[dim + 1];
	}
	// to get the index in the source array we first calculate the
	// indices and reproject
	index_defs += "{\nint working_index = index;\nindex = 0;\n";
	for (int dim = 0; dim < op.dimensions; dim++) {
		index_defs += "index += ((working_index /" +
					  to_string(acc_sizes_d[dim]) + ") % " +
					  to_string(op.shape[dim]) + ") * " +
					  to_string(acc_sizes_s[transposition[dim]]) + ";\n";
		index_defs += "working_index %= " + to_string(acc_sizes_d[dim]) + ";\n";
	}
	index_defs += "}\n";
	compiler_state.index_defs = index_defs;
	compiler_state.code.prepend("index = old_index" + to_string(old_idx) +
								";\n");
	compiler_state.code.prepend(
		"const " + type_string(node->operation.data_type) + " " + name +
		" = v" + to_string(compiler_state.variable_index + 1) + ";\n");
	return 0;
}
void TransposeImpl::execute_cpu(const FGraphNode *node,
								vector<CPUResultData> predecessor_data,
								void *__restrict__ result, size_t from,
								size_t size){UNARY_EXECUTE_MONOTON_IMPL}

FGraphNode *ConcatImpl::local_gradient(FGraphNode *y, int dx_i,
									   FGraphNode *prev_adj) {
	FGraphNode *a = y->predecessors[0];
	FGraphNode *b = y->predecessors[1];
	unsigned int ax = *((unsigned int *)y->operation.additional_data);
	if (0 == dx_i) {
		std::vector<long> start(a->operation.dimensions);
		std::vector<long> stop(a->operation.dimensions);
		for (int i = 0; i < a->operation.dimensions; i++) {
			start[i] = 0;
			stop[i] = a->operation.shape[i];
		}
		return fslice(prev_adj, start.data(), stop.data());
	} else if (1 == dx_i) {
		std::vector<long> start(b->operation.dimensions);
		std::vector<long> stop(b->operation.dimensions);
		for (int i = 0; i < b->operation.dimensions; i++) {
			start[i] = ax == i ? a->operation.shape[i] : 0;
			stop[i] = ax == i ? a->operation.shape[i] + b->operation.shape[i]
							  : b->operation.shape[i];
		}
		return fslice(prev_adj, start.data(), stop.data());
	} else
		return nullptr;
}
template <typename T>
void ConcatImpl::binary_expression(T *__restrict__ result,
								   const T *__restrict__ data1,
								   const T *__restrict__ data2, size_t from,
								   size_t size, size_t index_man_1,
								   size_t inv_man_1, size_t index_man_2,
								   size_t inv_man_2, const FGraphNode *curr) {
	FGraphNode *a = curr->predecessors[0];
	FGraphNode *b = curr->predecessors[1];
	unsigned int ax = ((unsigned int *)curr->operation.additional_data)[0];
	size_t acc_size_last = 1;
	for (int i = curr->operation.dimensions - 2; i >= (int)ax; i--) {
		acc_size_last *= curr->operation.shape[i + 1];
	}
	size_t total_el_size0 = 1;
	if (a->operation.op_type != FGEN_CONSTANT)
		for (int i = 0; i < a->operation.dimensions; i++)
			total_el_size0 *= a->operation.shape[i];
	size_t total_el_size1 = 1;
	if (b->operation.op_type != FGEN_CONSTANT)
		for (int i = 0; i < b->operation.dimensions; i++)
			total_el_size1 *= b->operation.shape[i];
	for (size_t index = from; index < from + size; index++) {
		size_t sx = index / acc_size_last;
		size_t sc = ax > 0 ? sx % curr->operation.shape[ax] : sx;
		// reproject to one of the tensors
		if (sc < a->operation.shape[ax]) {
			size_t ai = (sx / curr->operation.shape[ax]) * acc_size_last *
							a->operation.shape[ax] +
						sc * acc_size_last + index % acc_size_last;
			result[index] = data1[ai % total_el_size0];
		} else {
			size_t bi = (sx / curr->operation.shape[ax]) * acc_size_last *
							b->operation.shape[ax] +
						(sc - a->operation.shape[ax]) * acc_size_last +
						index % acc_size_last;
			result[index] = data2[bi % total_el_size1];
		}
	}
}
int ConcatImpl::generate_ocl_lazy(const FGraphNode *node, string name,
								  OCLLazyCodegenState &compiler_state) {
	FGraphNode *a = node->predecessors[0];
	FGraphNode *b = node->predecessors[1];
	const unsigned int old_idx = compiler_state.num_indices++;
	const unsigned int ax =
		((unsigned int *)node->operation.additional_data)[0];
	size_t acc_size_last = 1;
	for (int i = node->operation.dimensions - 2; i >= (int)ax; i--)
		acc_size_last *= node->operation.shape[i + 1];
	Twine index_defs;
	index_defs += "long old_index" + to_string(old_idx) + " = index;\n";
	const string sx = "index / " + to_string(acc_size_last);
	const string sc =
		ax > 0 ? "(" + sx + ") % " + to_string(node->operation.shape[ax]) : sx;
	const string if_em = sc + " < " + to_string(a->operation.shape[ax]);
	index_defs += "index = " + if_em +
				  " ? "
				  "(" +
				  sx + " / " + to_string(node->operation.shape[ax]) + ") * " +
				  to_string(acc_size_last * a->operation.shape[ax]) + " + (" +
				  sc + ") * " + to_string(acc_size_last) + " + (index % " +
				  to_string(acc_size_last) + "): (" + sx + " / " +
				  to_string(node->operation.shape[ax]) + ") * " +
				  to_string(acc_size_last * b->operation.shape[ax]) + " + ((" +
				  sc + ") - " + to_string(a->operation.shape[ax]) + ") * " +
				  to_string(acc_size_last) + " + (index % " +
				  to_string(acc_size_last) + ");\n";
	compiler_state.index_defs = index_defs;
	compiler_state.code.prepend(
		"index = old_index" + to_string(old_idx) + ";\n" +
		type_string(node->operation.data_type) + " " + name + " = " + if_em +
		" ? v" + to_string(compiler_state.variable_index + 1) + " : v" +
		to_string(compiler_state.variable_index + 2) + ";\n");
	return 0;
}
void ConcatImpl::execute_cpu(const FGraphNode *node,
							 vector<CPUResultData> predecessor_data,
							 void *__restrict__ result, size_t from,
							 size_t size) {
	BINARY_EXECUTE_MONOTON_IMPL
}
