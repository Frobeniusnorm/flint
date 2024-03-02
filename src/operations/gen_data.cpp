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
#include "gen_data.hpp"
#include "../utils.hpp"
#include <random>
using namespace std;
void GenRandomImpl::execute_cpu(const FGraphNode *node,
								std::vector<CPUResultData> predecessor_data,
								void *__restrict__ result, size_t from,
								size_t size) {
	double seed = ((double *)node->operation.additional_data)[0];
  std::minstd_rand0 g1 (seed * 1000 + from);
	for (size_t i = from; i < from + size; i++) {
		((double *)result)[i] = (g1() % 100000000) / 100000000.0;
	}
}
int GenRandomImpl::generate_ocl_lazy(const FGraphNode *node, std::string name,
									 OCLLazyCodegenState &compiler_state) {
	const string type = type_string(node->operation.data_type);
	double seed = ((double *)node->operation.additional_data)[0];
	compiler_state.code.prepend(type + " " + name + " = 0;\n{\n " + name +
								" = sin(index + " + std::to_string(seed) +
								") * 43758.5453123;\n " + name + " = min(" +
								name + " - floor(" + name +
								"), 0.99999);\n"
								"}\n");
	return 0;
}

template <typename T>
void GenConstantImpl::zeroary_expression(const FGraphNode *node,
										 T *__restrict__ result, size_t from,
										 size_t size) {
	T value = ((T *)node->operation.additional_data)[0];
	for (size_t i = from; i < from + size; i++)
		result[i] = value;
}
int GenConstantImpl::generate_ocl_lazy(const FGraphNode *node, std::string name,
									   OCLLazyCodegenState &compiler_state) {
	flogging(F_ERROR, "Constant Generation should not be implemented in OpenCL "
					  "code generation!");
	return 0;
}
void GenConstantImpl::execute_cpu(const FGraphNode *node,
								  std::vector<CPUResultData> predecessor_data,
								  void *__restrict__ result, size_t from,
								  size_t size) {
	ZEROARY_EXECUTE_IMPL
}
void GenArangeImpl::execute_cpu(const FGraphNode *node,
								std::vector<CPUResultData> predecessor_data,
								void *__restrict__ result, size_t from,
								size_t size) {
	unsigned int ax = ((unsigned int *)node->operation.additional_data)[0];
	size_t acc_sizes_ax = 1;
	for (unsigned int i = ax + 1; i < node->operation.dimensions; i++)
		acc_sizes_ax *= node->operation.shape[i];
	for (size_t i = from; i < from + size; i++)
		((long *)result)[i] = (i / acc_sizes_ax) % node->operation.shape[ax];
}
int GenArangeImpl::generate_ocl_lazy(const FGraphNode *node, std::string name,
									 OCLLazyCodegenState &compiler_state) {
	const string type = type_string(node->operation.data_type);
	unsigned int ax = ((unsigned int *)node->operation.additional_data)[0];
	size_t acc_sizes_ax = 1;
	for (unsigned int i = ax + 1; i < node->operation.dimensions; i++)
		acc_sizes_ax *= node->operation.shape[i];
	compiler_state.code.prepend("const " + type + " " + name + " = (index/" +
								to_string(acc_sizes_ax) + ")%" +
								to_string(node->operation.shape[ax]) + ";\n");
	return 0;
}
