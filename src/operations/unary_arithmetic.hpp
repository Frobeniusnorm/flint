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
#ifndef FLINT_UNARY_ARITHMETIC_HPP
#define FLINT_UNARY_ARITHMETIC_HPP
#include "implementation.hpp"

struct NegImpl : OperationImplementation {
		template <typename T>
		static void unary_expression(T *__restrict__ result,
									 const T *__restrict__ data1, size_t from,
									 size_t size, const FGraphNode *curr);
		void execute_cpu(const FGraphNode *node,
						 std::vector<CPUResultData> predecessor_data,
						 void *__restrict__ result, size_t from,
						 size_t size) override;
		void generate_gpu_lazy() override {}
		void generate_gpu_eager() override {}
};
struct LogImpl : OperationImplementation {
		template <typename T, typename A>
		static void unary_expression(T *__restrict__ result,
									 const A *__restrict__ data1, size_t from,
									 size_t size, const FGraphNode *curr);
		void execute_cpu(const FGraphNode *node,
						 std::vector<CPUResultData> predecessor_data,
						 void *__restrict__ result, size_t from,
						 size_t size) override;
		void generate_gpu_lazy() override {}
		void generate_gpu_eager() override {}
};
struct Log2Impl : OperationImplementation {
		template <typename T, typename A>
		static void unary_expression(T *__restrict__ result,
									 const A *__restrict__ data1, size_t from,
									 size_t size, const FGraphNode *curr);
		void execute_cpu(const FGraphNode *node,
						 std::vector<CPUResultData> predecessor_data,
						 void *__restrict__ result, size_t from,
						 size_t size) override;
		void generate_gpu_lazy() override {}
		void generate_gpu_eager() override {}
};
struct Log10Impl : OperationImplementation {
		template <typename T, typename A>
		static void unary_expression(T *__restrict__ result,
									 const A *__restrict__ data1, size_t from,
									 size_t size, const FGraphNode *curr);
		void execute_cpu(const FGraphNode *node,
						 std::vector<CPUResultData> predecessor_data,
						 void *__restrict__ result, size_t from,
						 size_t size) override;
		void generate_gpu_lazy() override {}
		void generate_gpu_eager() override {}
};
struct SignImpl : OperationImplementation {
		template <typename A>
		static void unary_expression(int *__restrict__ result,
									 const A *__restrict__ data1, size_t from,
									 size_t size, const FGraphNode *curr);
		void execute_cpu(const FGraphNode *node,
						 std::vector<CPUResultData> predecessor_data,
						 void *__restrict__ result, size_t from,
						 size_t size) override;
		void generate_gpu_lazy() override {}
		void generate_gpu_eager() override {}
};
struct EvenImpl : OperationImplementation {
		void execute_cpu(const FGraphNode *node,
						 std::vector<CPUResultData> predecessor_data,
						 void *__restrict__ result, size_t from,
						 size_t size) override;
		void generate_gpu_lazy() override {}
		void generate_gpu_eager() override {}
};
struct SinImpl : OperationImplementation {
		template <typename T, typename A>
		static void unary_expression(T *__restrict__ result,
									 const A *__restrict__ data1, size_t from,
									 size_t size, const FGraphNode *curr);
		void execute_cpu(const FGraphNode *node,
						 std::vector<CPUResultData> predecessor_data,
						 void *__restrict__ result, size_t from,
						 size_t size) override;
		void generate_gpu_lazy() override {}
		void generate_gpu_eager() override {}
};
struct CosImpl : OperationImplementation {
		template <typename T, typename A>
		static void unary_expression(T *__restrict__ result,
									 const A *__restrict__ data1, size_t from,
									 size_t size, const FGraphNode *curr);
		void execute_cpu(const FGraphNode *node,
						 std::vector<CPUResultData> predecessor_data,
						 void *__restrict__ result, size_t from,
						 size_t size) override;
		void generate_gpu_lazy() override {}
		void generate_gpu_eager() override {}
};
struct TanImpl : OperationImplementation {
		template <typename T, typename A>
		static void unary_expression(T *__restrict__ result,
									 const A *__restrict__ data1, size_t from,
									 size_t size, const FGraphNode *curr);
		void execute_cpu(const FGraphNode *node,
						 std::vector<CPUResultData> predecessor_data,
						 void *__restrict__ result, size_t from,
						 size_t size) override;
		void generate_gpu_lazy() override {}
		void generate_gpu_eager() override {}
};
struct ASinImpl : OperationImplementation {
		template <typename T, typename A>
		static void unary_expression(T *__restrict__ result,
									 const A *__restrict__ data1, size_t from,
									 size_t size, const FGraphNode *curr);
		void execute_cpu(const FGraphNode *node,
						 std::vector<CPUResultData> predecessor_data,
						 void *__restrict__ result, size_t from,
						 size_t size) override;
		void generate_gpu_lazy() override {}
		void generate_gpu_eager() override {}
};
struct ACosImpl : OperationImplementation {
		template <typename T, typename A>
		static void unary_expression(T *__restrict__ result,
									 const A *__restrict__ data1, size_t from,
									 size_t size, const FGraphNode *curr);
		void execute_cpu(const FGraphNode *node,
						 std::vector<CPUResultData> predecessor_data,
						 void *__restrict__ result, size_t from,
						 size_t size) override;
		void generate_gpu_lazy() override {}
		void generate_gpu_eager() override {}
};
struct ATanImpl : OperationImplementation {
		template <typename T, typename A>
		static void unary_expression(T *__restrict__ result,
									 const A *__restrict__ data1, size_t from,
									 size_t size, const FGraphNode *curr);
		void execute_cpu(const FGraphNode *node,
						 std::vector<CPUResultData> predecessor_data,
						 void *__restrict__ result, size_t from,
						 size_t size) override;
		void generate_gpu_lazy() override {}
		void generate_gpu_eager() override {}
};
struct SqrtImpl : OperationImplementation {
		template <typename T, typename A>
		static void unary_expression(T *__restrict__ result,
									 const A *__restrict__ data1, size_t from,
									 size_t size, const FGraphNode *curr);
		void execute_cpu(const FGraphNode *node,
						 std::vector<CPUResultData> predecessor_data,
						 void *__restrict__ result, size_t from,
						 size_t size) override;
		void generate_gpu_lazy() override {}
		void generate_gpu_eager() override {}
};
struct ExpImpl : OperationImplementation {
		template <typename T, typename A>
		static void unary_expression(T *__restrict__ result,
									 const A *__restrict__ data1, size_t from,
									 size_t size, const FGraphNode *curr);
		void execute_cpu(const FGraphNode *node,
						 std::vector<CPUResultData> predecessor_data,
						 void *__restrict__ result, size_t from,
						 size_t size) override;
		void generate_gpu_lazy() override {}
		void generate_gpu_eager() override {}
};
struct AbsImpl : OperationImplementation {
		template <typename T>
		static void unary_expression(T *__restrict__ result,
									 const T *__restrict__ data1, size_t from,
									 size_t size, const FGraphNode *curr);
		void execute_cpu(const FGraphNode *node,
						 std::vector<CPUResultData> predecessor_data,
						 void *__restrict__ result, size_t from,
						 size_t size) override;
		void generate_gpu_lazy() override {}
		void generate_gpu_eager() override {}
};
#endif