#ifndef FLINT_SHAPE_MODIFICATION_HPP
#define FLINT_SHAPE_MODIFICATION_HPP
#include "implementation.hpp"

struct FlattenImpl : OperationImplementation {
		void execute_cpu(const FGraphNode *node,
						 std::vector<CPUResultData> predecessor_data,
						 void *__restrict__ result, size_t from,
						 size_t size) override;
		void generate_gpu_lazy() override {}
		void generate_gpu_eager() override {}
};
struct ConversionImpl : OperationImplementation {
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
struct RepeatImpl : OperationImplementation {
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
struct TransposeImpl : OperationImplementation {
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
struct ConcatImpl : OperationImplementation {
		template <typename T>
		static void binary_expression(T *__restrict__ result,
									  const T *__restrict__ data1,
									  const T *__restrict__ data2, size_t from,
									  size_t size, size_t index_man_1,
									  size_t inv_man_1, size_t index_man_2,
									  size_t inv_man_2, const FGraphNode *curr);
		void execute_cpu(const FGraphNode *node,
						 std::vector<CPUResultData> predecessor_data,
						 void *__restrict__ result, size_t from,
						 size_t size) override;
		void generate_gpu_lazy() override {}
		void generate_gpu_eager() override {}
};
#endif
