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
#include "comparison.hpp"
#include "src/operations/implementation.hpp"

#define MIN_VAL(x, y) (x < y ? x : y)
#define MAX_VAL(x, y) (x < y ? y : x)

template <typename T, typename A, typename B>
void MinImpl::binary_expression(T *__restrict__ result,
								const A *__restrict__ data1,
								const B *__restrict__ data2, size_t from,
								size_t size, size_t index_man_1,
								size_t inv_man_1, size_t index_man_2,
								size_t inv_man_2, const FGraphNode *curr) {
	for (size_t i = from; i < from + size; i++)
		result[i] = MIN_VAL(data1[(i / inv_man_1) % index_man_1],
							data2[(i / inv_man_2) % index_man_2]);
}
void MinImpl::execute_cpu(const FGraphNode *node,
						  std::vector<CPUResultData> predecessor_data,
						  void *__restrict__ result, size_t from, size_t size) {
	BINARY_EXECUTE_IMPL
}
template <typename T, typename A, typename B>
void MaxImpl::binary_expression(T *__restrict__ result,
								const A *__restrict__ data1,
								const B *__restrict__ data2, size_t from,
								size_t size, size_t index_man_1,
								size_t inv_man_1, size_t index_man_2,
								size_t inv_man_2, const FGraphNode *curr) {
	for (size_t i = from; i < from + size; i++)
		result[i] = MAX_VAL(data1[(i / inv_man_1) % index_man_1],
							data2[(i / inv_man_2) % index_man_2]);
}
void MaxImpl::execute_cpu(const FGraphNode *node,
						  std::vector<CPUResultData> predecessor_data,
						  void *__restrict__ result, size_t from, size_t size) {
	BINARY_EXECUTE_IMPL
}
template <typename A, typename B>
void LessImpl::binary_expression(int *__restrict__ result,
								 const A *__restrict__ data1,
								 const B *__restrict__ data2, size_t from,
								 size_t size, size_t index_man_1,
								 size_t inv_man_1, size_t index_man_2,
								 size_t inv_man_2, const FGraphNode *curr) {
	for (size_t i = from; i < from + size; i++)
		result[i] = data1[(i / inv_man_1) % index_man_1] <
							data2[(i / inv_man_2) % index_man_2]
						? 1
						: 0;
}
void LessImpl::execute_cpu(const FGraphNode *node,
						   std::vector<CPUResultData> predecessor_data,
						   void *__restrict__ result, size_t from,
						   size_t size) {
	DISPATCH_BINARY_OPERATION(int)
}
template <typename A, typename B>
void GreaterImpl::binary_expression(int *__restrict__ result,
									const A *__restrict__ data1,
									const B *__restrict__ data2, size_t from,
									size_t size, size_t index_man_1,
									size_t inv_man_1, size_t index_man_2,
									size_t inv_man_2, const FGraphNode *curr) {
	for (size_t i = from; i < from + size; i++)
		result[i] = data1[(i / inv_man_1) % index_man_1] >
							data2[(i / inv_man_2) % index_man_2]
						? 1
						: 0;
}
void GreaterImpl::execute_cpu(const FGraphNode *node,
							  std::vector<CPUResultData> predecessor_data,
							  void *__restrict__ result, size_t from,
							  size_t size) {
	DISPATCH_BINARY_OPERATION(int)
}
template <typename A, typename B>
void EqualImpl::binary_expression(int *__restrict__ result,
								  const A *__restrict__ data1,
								  const B *__restrict__ data2, size_t from,
								  size_t size, size_t index_man_1,
								  size_t inv_man_1, size_t index_man_2,
								  size_t inv_man_2, const FGraphNode *curr) {
	for (size_t i = from; i < from + size; i++)
		result[i] = data1[(i / inv_man_1) % index_man_1] ==
							data2[(i / inv_man_2) % index_man_2]
						? 1
						: 0;
}
void EqualImpl::execute_cpu(const FGraphNode *node,
							std::vector<CPUResultData> predecessor_data,
							void *__restrict__ result, size_t from,
							size_t size) {
	DISPATCH_BINARY_OPERATION(int)
}