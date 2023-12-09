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
#include "unary_arithmetic.hpp"

using namespace std;

template <typename T>
void NegImpl::unary_expression(T *__restrict__ result,
							   const T *__restrict__ data, size_t from,
							   size_t size, const FGraphNode *curr) {
	for (size_t i = from; i < from + size; i++)
		result[i] = -data[i];
}
int NegImpl::generate_ocl_lazy(const FGraphNode *node, string name,
							   OCLLazyCodegenState &compiler_state) {
	compiler_state.code.prepend(
		"const " + typeString(node->operation.data_type) + " " + name +
		" = -v" + to_string(compiler_state.variable_index + 1) + ";\n");
	return 0;
}
void NegImpl::execute_cpu(const FGraphNode *node,
						  vector<CPUResultData> predecessor_data,
						  void *__restrict__ result, size_t from, size_t size) {
	UNARY_EXECUTE_MONOTON_IMPL
}
template <typename T, typename A>
void LogImpl::unary_expression(T *__restrict__ result,
							   const A *__restrict__ data, size_t from,
							   size_t size, const FGraphNode *curr) {
	for (size_t i = from; i < from + size; i++)
		result[i] = log(data[i]);
}
int LogImpl::generate_ocl_lazy(const FGraphNode *node, string name,
							   OCLLazyCodegenState &compiler_state) {
	compiler_state.code.prepend(
		"const " + typeString(node->operation.data_type) + " " + name +
		" = log(v" + to_string(compiler_state.variable_index + 1) + ");\n");
	return 0;
}
void LogImpl::execute_cpu(const FGraphNode *node,
						  vector<CPUResultData> predecessor_data,
						  void *__restrict__ result, size_t from, size_t size) {
	UNARY_EXECUTE_IMPL
}
template <typename T, typename A>
void Log2Impl::unary_expression(T *__restrict__ result,
								const A *__restrict__ data, size_t from,
								size_t size, const FGraphNode *curr) {
	for (size_t i = from; i < from + size; i++)
		result[i] = log2(data[i]);
}
int Log2Impl::generate_ocl_lazy(const FGraphNode *node, string name,
								OCLLazyCodegenState &compiler_state) {
	compiler_state.code.prepend(
		"const " + typeString(node->operation.data_type) + " " + name +
		" = log2(v" + to_string(compiler_state.variable_index + 1) + ");\n");
	return 0;
}
void Log2Impl::execute_cpu(const FGraphNode *node,
						   vector<CPUResultData> predecessor_data,
						   void *__restrict__ result, size_t from,
						   size_t size) {
	UNARY_EXECUTE_IMPL
}
template <typename T, typename A>
void Log10Impl::unary_expression(T *__restrict__ result,
								 const A *__restrict__ data, size_t from,
								 size_t size, const FGraphNode *curr) {
	for (size_t i = from; i < from + size; i++)
		result[i] = log10(data[i]);
}
int Log10Impl::generate_ocl_lazy(const FGraphNode *node, string name,
								 OCLLazyCodegenState &compiler_state) {
	compiler_state.code.prepend(
		"const " + typeString(node->operation.data_type) + " " + name +
		" = log10(v" + to_string(compiler_state.variable_index + 1) + ");\n");
	return 0;
}
void Log10Impl::execute_cpu(const FGraphNode *node,
							vector<CPUResultData> predecessor_data,
							void *__restrict__ result, size_t from,
							size_t size) {
	UNARY_EXECUTE_IMPL
}
template <typename A>
void SignImpl::unary_expression(int *__restrict result,
								const A *__restrict data1, size_t from,
								size_t size, const FGraphNode *curr) {
	for (size_t i = from; i < from + size; i++)
		result[i] = data1[i] < 0 ? -1 : 1;
}
int SignImpl::generate_ocl_lazy(const FGraphNode *node, string name,
								OCLLazyCodegenState &compiler_state) {

	compiler_state.code.prepend(
		"const " + typeString(node->operation.data_type) + " " + name + " = v" +
		to_string(compiler_state.variable_index + 1) + " < 0 ? -1 : 1;\n");
	return 0;
}
void SignImpl::execute_cpu(const FGraphNode *node,
						   vector<CPUResultData> predecessor_data,
						   void *__restrict__ result, size_t from,
						   size_t size) {
	DISPATCH_UNARY_OPERATION(int)
}
void EvenImpl::execute_cpu(const FGraphNode *node,
						   vector<CPUResultData> predecessor_data,
						   void *__restrict__ result, size_t from,
						   size_t size) {
	for (size_t i = from; i < from + size; i++)
		switch (predecessor_data[0].type) {
		case F_INT32:
			((int *__restrict__)result)[i] =
				((int *__restrict__)predecessor_data[0].data)[i] % 2 ? 0 : 1;
			break;
		case F_INT64:
			((int *__restrict__)result)[i] =
				((long *__restrict__)predecessor_data[0].data)[i] % 2 ? 0 : 1;
			break;
		case F_FLOAT32:
			((int *__restrict__)result)[i] = 0;
			break;
		case F_FLOAT64:
			((int *__restrict__)result)[i] = 0;
			break;
		}
}
int EvenImpl::generate_ocl_lazy(const FGraphNode *node, string name,
								OCLLazyCodegenState &compiler_state) {
	compiler_state.code.prepend(
		"const " + typeString(node->operation.data_type) + " " + name + " = v" +
		to_string(compiler_state.variable_index + 1) + " % 2 == 0 ? 1 : 0;\n");
	return 0;
}
template <typename T, typename A>
void SinImpl::unary_expression(T *__restrict__ result,
							   const A *__restrict__ data, size_t from,
							   size_t size, const FGraphNode *curr) {
	for (size_t i = from; i < from + size; i++)
		result[i] = sin(data[i]);
}
int SinImpl::generate_ocl_lazy(const FGraphNode *node, string name,
							   OCLLazyCodegenState &compiler_state) {
	compiler_state.code.prepend(
		"const " + typeString(node->operation.data_type) + " " + name +
		" = sin(v" + to_string(compiler_state.variable_index + 1) + ");\n");
	return 0;
}
void SinImpl::execute_cpu(const FGraphNode *node,
						  vector<CPUResultData> predecessor_data,
						  void *__restrict__ result, size_t from, size_t size) {
	UNARY_EXECUTE_IMPL
}
template <typename T, typename A>
void CosImpl::unary_expression(T *__restrict__ result,
							   const A *__restrict__ data, size_t from,
							   size_t size, const FGraphNode *curr) {
	for (size_t i = from; i < from + size; i++)
		result[i] = cos(data[i]);
}
int CosImpl::generate_ocl_lazy(const FGraphNode *node, string name,
							   OCLLazyCodegenState &compiler_state) {
	compiler_state.code.prepend(
		"const " + typeString(node->operation.data_type) + " " + name +
		" = cos(v" + to_string(compiler_state.variable_index + 1) + ");\n");
	return 0;
}
void CosImpl::execute_cpu(const FGraphNode *node,
						  vector<CPUResultData> predecessor_data,
						  void *__restrict__ result, size_t from, size_t size) {
	UNARY_EXECUTE_IMPL
}
template <typename T, typename A>
void TanImpl::unary_expression(T *__restrict__ result,
							   const A *__restrict__ data, size_t from,
							   size_t size, const FGraphNode *curr) {
	for (size_t i = from; i < from + size; i++)
		result[i] = tan(data[i]);
}
int TanImpl::generate_ocl_lazy(const FGraphNode *node, string name,
							   OCLLazyCodegenState &compiler_state) {
	compiler_state.code.prepend(
		"const " + typeString(node->operation.data_type) + " " + name +
		" = tan(v" + to_string(compiler_state.variable_index + 1) + ");\n");
	return 0;
}
void TanImpl::execute_cpu(const FGraphNode *node,
						  vector<CPUResultData> predecessor_data,
						  void *__restrict__ result, size_t from, size_t size) {
	UNARY_EXECUTE_IMPL
}
template <typename T, typename A>
void ASinImpl::unary_expression(T *__restrict__ result,
								const A *__restrict__ data, size_t from,
								size_t size, const FGraphNode *curr) {
	for (size_t i = from; i < from + size; i++)
		result[i] = asin(data[i]);
}
int ASinImpl::generate_ocl_lazy(const FGraphNode *node, string name,
								OCLLazyCodegenState &compiler_state) {
	compiler_state.code.prepend(
		"const " + typeString(node->operation.data_type) + " " + name +
		" = asin(v" + to_string(compiler_state.variable_index + 1) + ");\n");
	return 0;
}
void ASinImpl::execute_cpu(const FGraphNode *node,
						   vector<CPUResultData> predecessor_data,
						   void *__restrict__ result, size_t from,
						   size_t size) {
	UNARY_EXECUTE_IMPL
}
template <typename T, typename A>
void ACosImpl::unary_expression(T *__restrict__ result,
								const A *__restrict__ data, size_t from,
								size_t size, const FGraphNode *curr) {
	for (size_t i = from; i < from + size; i++)
		result[i] = acos(data[i]);
}
int ACosImpl::generate_ocl_lazy(const FGraphNode *node, string name,
								OCLLazyCodegenState &compiler_state) {
	compiler_state.code.prepend(
		"const " + typeString(node->operation.data_type) + " " + name +
		" = acos(v" + to_string(compiler_state.variable_index + 1) + ");\n");
	return 0;
}
void ACosImpl::execute_cpu(const FGraphNode *node,
						   vector<CPUResultData> predecessor_data,
						   void *__restrict__ result, size_t from,
						   size_t size) {
	UNARY_EXECUTE_IMPL
}
template <typename T, typename A>
void ATanImpl::unary_expression(T *__restrict__ result,
								const A *__restrict__ data, size_t from,
								size_t size, const FGraphNode *curr) {
	for (size_t i = from; i < from + size; i++)
		result[i] = atan(data[i]);
}
int ATanImpl::generate_ocl_lazy(const FGraphNode *node, string name,
								OCLLazyCodegenState &compiler_state) {
	compiler_state.code.prepend(
		"const " + typeString(node->operation.data_type) + " " + name +
		" = atan(v" + to_string(compiler_state.variable_index + 1) + ");\n");
	return 0;
}
void ATanImpl::execute_cpu(const FGraphNode *node,
						   vector<CPUResultData> predecessor_data,
						   void *__restrict__ result, size_t from,
						   size_t size) {
	UNARY_EXECUTE_IMPL
}
template <typename T, typename A>
void SqrtImpl::unary_expression(T *__restrict__ result,
								const A *__restrict__ data, size_t from,
								size_t size, const FGraphNode *curr) {
	for (size_t i = from; i < from + size; i++)
		result[i] = sqrt(data[i]);
}
int SqrtImpl::generate_ocl_lazy(const FGraphNode *node, string name,
								OCLLazyCodegenState &compiler_state) {
	compiler_state.code.prepend(
		"const " + typeString(node->operation.data_type) + " " + name +
		" = sqrt(v" + to_string(compiler_state.variable_index + 1) + ");\n");
	return 0;
}
void SqrtImpl::execute_cpu(const FGraphNode *node,
						   vector<CPUResultData> predecessor_data,
						   void *__restrict__ result, size_t from,
						   size_t size) {
	UNARY_EXECUTE_IMPL
}
template <typename T, typename A>
void ExpImpl::unary_expression(T *__restrict__ result,
							   const A *__restrict__ data, size_t from,
							   size_t size, const FGraphNode *curr) {
	for (size_t i = from; i < from + size; i++)
		result[i] = exp(data[i]);
}
int ExpImpl::generate_ocl_lazy(const FGraphNode *node, string name,
							   OCLLazyCodegenState &compiler_state) {
	compiler_state.code.prepend(
		"const " + typeString(node->operation.data_type) + " " + name +
		" = exp(v" + to_string(compiler_state.variable_index + 1) + ");\n");
	return 0;
}
void ExpImpl::execute_cpu(const FGraphNode *node,
						  vector<CPUResultData> predecessor_data,
						  void *__restrict__ result, size_t from, size_t size) {
	UNARY_EXECUTE_IMPL
}
template <typename T>
void AbsImpl::unary_expression(T *__restrict__ result,
							   const T *__restrict__ data, size_t from,
							   size_t size, const FGraphNode *curr) {
	for (size_t i = from; i < from + size; i++)
		result[i] = abs(data[i]);
}
int AbsImpl::generate_ocl_lazy(const FGraphNode *node, string name,
							   OCLLazyCodegenState &compiler_state) {
	compiler_state.code.prepend(
		"const " + typeString(node->operation.data_type) + " " + name + " = v" +
		to_string(compiler_state.variable_index + 1) + " < 0 ? -v" +
		to_string(compiler_state.variable_index + 1) + ": v" +
		to_string(compiler_state.variable_index + 1) + ";\n");
	return 0;
}
void AbsImpl::execute_cpu(const FGraphNode *node,
						  vector<CPUResultData> predecessor_data,
						  void *__restrict__ result, size_t from, size_t size) {
	UNARY_EXECUTE_MONOTON_IMPL
}
