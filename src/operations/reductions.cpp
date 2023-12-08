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
#include "reductions.hpp"
#include "flint.h"
#include <limits>

#define MIN_VAL(x, y) (x < y ? x : y)
#define MAX_VAL(x, y) (x < y ? y : x)

using namespace std;

template <FOperationType op_type>
static inline int reducing(const FGraphNode *node, std::string name,
						   OCLLazyCodegenState &compiler_state) {
	FGraphNode *prev = node->predecessors[0];
	// we insert a index definition to introduce the for for our
	// predecessors
	const string par1 = "v" + std::to_string(compiler_state.variable_index + 1);
	const string type = typeString(node->operation.data_type);
	const int red_dim = ((int *)node->operation.additional_data)[0];
	size_t it_dim = 1; // iteration size <=> product of all dimensions along dim
	for (size_t d = red_dim + 1; d < prev->operation.dimensions; d++)
		it_dim *= prev->operation.shape[d];
	Twine index_defs;
	index_defs += type + " " + name + " = ";
	size_t total_el_size = 1;
	for (int i = 0; i < prev->operation.dimensions; i++)
		total_el_size *= prev->operation.shape[i];
	switch (op_type) {
	case FREDUCE_SUM:
		index_defs += "0";
		break;
	case FREDUCE_MUL:
		index_defs += "1";
		break;
	case FREDUCE_MIN:
		index_defs += maxForType(node->operation.data_type);
		break;
	case FREDUCE_MAX:
		index_defs += minForType(node->operation.data_type);
		break;
	default:
		break;
	}
	const std::string itv = "i" + to_string(compiler_state.variable_index);
	const unsigned int old_idx = compiler_state.num_indices++;
	index_defs += ";\nlong old_idx" + to_string(old_idx) +
				  " = index;\n"
				  "for(long " +
				  itv + " = 0; " + itv + " < " +
				  to_string(prev->operation.shape[red_dim]) + "; " + itv +
				  "++){\n"
				  "index = ((old_idx" +
				  to_string(old_idx) + " / " + to_string(it_dim) + ") * " +

				  to_string(it_dim) + " * " +
				  to_string(prev->operation.shape[red_dim]) + " + (old_idx" +
				  to_string(old_idx) + " % " + to_string(it_dim) + ") + " +
				  itv + " * " + to_string(it_dim) + ") % " +
				  to_string(total_el_size) + ";\n";
	compiler_state.index_defs = index_defs;
	Twine reduce_code;
	switch (op_type) {
	case FREDUCE_SUM:
		reduce_code += " " + name + " += " + par1;
		break;
	case FREDUCE_MUL:
		reduce_code += " " + name + " *= " + par1;
		break;
	case FREDUCE_MIN:
		reduce_code += " " + name + " = min(" + name + ", " + par1 + ")";
		break;
	case FREDUCE_MAX:
		reduce_code += " " + name + " = max(" + name + ", " + par1 + ")";
		break;
	default:
		break;
	}
	reduce_code += ";\n}\nindex = old_idx" + to_string(old_idx) + ";\n";
	compiler_state.code.prepend(reduce_code);
	return 0;
}

template <typename T>
void ReduceSumImpl::unary_expression(T *__restrict__ result,
									 const T *__restrict__ data, size_t from,
									 size_t size, const FGraphNode *curr) {
	const FOperation pred = curr->predecessors[0]->operation;
	const int dim = ((int *)curr->operation.additional_data)[0];
	size_t it_dim = 1; // iteration size <=> product of all dimensions along dim
	for (size_t d = dim + 1; d < pred.dimensions; d++)
		it_dim *= pred.shape[d];
	for (size_t i = from; i < from + size; i++) {
		result[i] = 0;
		for (size_t j = 0; j < pred.shape[dim]; j++) {
			const T curr = data[(i / it_dim) * it_dim * pred.shape[dim] +
								i % it_dim + j * it_dim];
			result[i] += curr;
		}
	}
}
int ReduceSumImpl::generate_ocl_lazy(const FGraphNode *node, std::string name,
									 OCLLazyCodegenState &compiler_state) {
	return reducing<FREDUCE_SUM>(node, name, compiler_state);
}
template <typename T>
void ReduceMulImpl::unary_expression(T *__restrict__ result,
									 const T *__restrict__ data, size_t from,
									 size_t size, const FGraphNode *curr) {
	const FOperation pred = curr->predecessors[0]->operation;
	const int dim = ((int *)curr->operation.additional_data)[0];
	size_t it_dim = 1; // iteration size <=> product of all dimensions along dim
	for (size_t d = dim + 1; d < pred.dimensions; d++)
		it_dim *= pred.shape[d];
	for (size_t i = from; i < from + size; i++) {
		result[i] = 1;
		for (size_t j = 0; j < pred.shape[dim]; j++) {
			const T curr = data[(i / it_dim) * it_dim * pred.shape[dim] +
								i % it_dim + j * it_dim];
			result[i] *= curr;
		}
	}
}
int ReduceMulImpl::generate_ocl_lazy(const FGraphNode *node, std::string name,
									 OCLLazyCodegenState &compiler_state) {
	return reducing<FREDUCE_MUL>(node, name, compiler_state);
}
template <typename T>
void ReduceMinImpl::unary_expression(T *__restrict__ result,
									 const T *__restrict__ data, size_t from,
									 size_t size, const FGraphNode *curr) {
	const FOperation pred = curr->predecessors[0]->operation;
	const int dim = ((int *)curr->operation.additional_data)[0];
	size_t it_dim = 1; // iteration size <=> product of all dimensions along dim
	for (size_t d = dim + 1; d < pred.dimensions; d++)
		it_dim *= pred.shape[d];
	for (size_t i = from; i < from + size; i++) {
		result[i] = std::numeric_limits<T>::max();
		for (size_t j = 0; j < pred.shape[dim]; j++) {
			const T curr = data[(i / it_dim) * it_dim * pred.shape[dim] +
								i % it_dim + j * it_dim];
			result[i] = MIN_VAL(result[i], curr);
		}
	}
}
int ReduceMinImpl::generate_ocl_lazy(const FGraphNode *node, std::string name,
									 OCLLazyCodegenState &compiler_state) {
	return reducing<FREDUCE_MIN>(node, name, compiler_state);
}
template <typename T>
void ReduceMaxImpl::unary_expression(T *__restrict__ result,
									 const T *__restrict__ data, size_t from,
									 size_t size, const FGraphNode *curr) {
	const FOperation pred = curr->predecessors[0]->operation;
	const int dim = ((int *)curr->operation.additional_data)[0];
	size_t it_dim = 1; // iteration size <=> product of all dimensions along dim
	for (size_t d = dim + 1; d < pred.dimensions; d++)
		it_dim *= pred.shape[d];
	for (size_t i = from; i < from + size; i++) {
		result[i] = std::numeric_limits<T>::lowest();
		for (size_t j = 0; j < pred.shape[dim]; j++) {
			const T curr = data[(i / it_dim) * it_dim * pred.shape[dim] +
								i % it_dim + j * it_dim];
			result[i] = MAX_VAL(result[i], curr);
		}
	}
}
int ReduceMaxImpl::generate_ocl_lazy(const FGraphNode *node, std::string name,
									 OCLLazyCodegenState &compiler_state) {
	return reducing<FREDUCE_MAX>(node, name, compiler_state);
}
void ReduceSumImpl::execute_cpu(const FGraphNode *node,
								std::vector<CPUResultData> predecessor_data,
								void *__restrict__ result, size_t from,
								size_t size) {
	UNARY_EXECUTE_MONOTON_IMPL
}
void ReduceMulImpl::execute_cpu(const FGraphNode *node,
								std::vector<CPUResultData> predecessor_data,
								void *__restrict__ result, size_t from,
								size_t size) {
	UNARY_EXECUTE_MONOTON_IMPL
}
void ReduceMinImpl::execute_cpu(const FGraphNode *node,
								std::vector<CPUResultData> predecessor_data,
								void *__restrict__ result, size_t from,
								size_t size) {
	UNARY_EXECUTE_MONOTON_IMPL
}
void ReduceMaxImpl::execute_cpu(const FGraphNode *node,
								std::vector<CPUResultData> predecessor_data,
								void *__restrict__ result, size_t from,
								size_t size) {
	UNARY_EXECUTE_MONOTON_IMPL
}
