/* Copyright 2022 David Schwarzbeck

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License. */

#ifndef FLINT_H
#define FLINT_H
#include "src/logger.hpp"
#define CL_TARGET_OPENCL_VERSION 200
#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif
#ifdef __cplusplus
extern "C" {
#endif
/*
  This is the basic header file and implementation of Flint, to be compatible
  with as many languages as possible. I have never tested it with the bare C
  compiler, but it should be C compatible.

  Flints Execution structure represents an AST, so that each Graph may be
  compiled to a specific OpenCL program
*/

// initializes flint, if cpu is set the cpu backend is initialized and used, if
// gpu is set the gpu backend is initialized and used
void flintInit(int cpu, int gpu);
void flintInit_cpu();
void flintInit_gpu();
void flintCleanup();
void flintCleanup_cpu();
void flintCleanup_gpu();
// 0 no logging, 1 only errors, 2 errors and warnings, 3 error, warnings and
// info, 4 verbose
void setLoggingLevel(int);

enum FType { INT32, INT64, FLOAT32, FLOAT64 };
enum FOperationType {
  STORE,
  RESULTDATA,
  CONST,
  ADD,
  SUB,
  MUL,
  DIV,
  POW,
  FLATTEN,
  MATMUL,
  Length
};
struct FOperation {
  // shape of the data after execution
  int dimensions;
  int *shape;
  // type of operation, to enable switch cases and avoid v-table lookups
  FOperationType op_type;
  // datatype of result
  FType data_type;
  void *additional_data;
};
// each Graph Node represents one Operation with multiple ingoing edges and one
// out going edge
struct FGraphNode {
  int num_predecessor;
  FGraphNode **predecessors;
  FOperation *operation;    // the operation represented by this graph node
  size_t reference_counter; // for garbage collection in free graph
};
// Operations
// Store data in the tensor, always an Entry (source) of the graph
struct FStore {
  // link to gpu data
  cl_mem mem_id = nullptr;
  void *data;
  int num_entries;
};
struct FResultData {
  // link to gpu data
  cl_mem mem_id = nullptr;
  void *data;
  int num_entries;
};
// A single-value constant (does not have to be loaded as a tensor)
struct FConst {
  void *value; // has to be one of Type
};

// functions

// creates a Graph with a single store instruction, data is copied to intern
// memory, so after return of the function, data may be deleted. Shape is also
// instantly copied. Data contains the flattened data array from type data_type
// and shape contains the size on each dimension as an array with length
// dimensions.
FGraphNode *createGraph(void *data, int num_entries, FType data_type,
                        int *shape, int dimensions);
// frees all allocated data from the graph and the nodes that are reachable
void freeGraph(FGraphNode *graph);

FGraphNode *copyGraph(const FGraphNode *graph);
/* executes the Graph which starts with the root of the given node and stores
 * the result for that node in result. If Flint was not initialized yet, this
 * function will do that automatically.
 * This function will automatically determine if the graph is executed on the
 * cpu or on the gpu. The result data is automatically freed with the Graph */
FGraphNode *executeGraph(FGraphNode *node);
FGraphNode *executeGraph_cpu(FGraphNode *node);
FGraphNode *executeGraph_gpu(FGraphNode *node);
// operations
FGraphNode *add_g(FGraphNode *a, FGraphNode *b);
FGraphNode *sub_g(FGraphNode *a, FGraphNode *b);
FGraphNode *div_g(FGraphNode *a, FGraphNode *b);
FGraphNode *mul_g(FGraphNode *a, FGraphNode *b);
FGraphNode *pow_g(FGraphNode *a, FGraphNode *b);
// adds the constant value to each entry in a
FGraphNode *add_ci(FGraphNode *a, const int b);
FGraphNode *add_cl(FGraphNode *a, const long b);
FGraphNode *add_cf(FGraphNode *a, const float b);
FGraphNode *add_cd(FGraphNode *a, const double b);
// subtracts the constant value from each entry in a
FGraphNode *sub_ci(FGraphNode *a, const int b);
FGraphNode *sub_cl(FGraphNode *a, const long b);
FGraphNode *sub_cf(FGraphNode *a, const float b);
FGraphNode *sub_cd(FGraphNode *a, const double b);
// divides each entry in a by the constant value
FGraphNode *div_ci(FGraphNode *a, const int b);
FGraphNode *div_cl(FGraphNode *a, const long b);
FGraphNode *div_cf(FGraphNode *a, const float b);
FGraphNode *div_cd(FGraphNode *a, const double b);
// multiplicates the constant value with each entry in a
FGraphNode *mul_ci(FGraphNode *a, const int b);
FGraphNode *mul_cl(FGraphNode *a, const long b);
FGraphNode *mul_cf(FGraphNode *a, const float b);
FGraphNode *mul_cd(FGraphNode *a, const double b);
// computes the power of each entry to the constant
FGraphNode *pow_ci(FGraphNode *a, const int b);
FGraphNode *pow_cl(FGraphNode *a, const long b);
FGraphNode *pow_cf(FGraphNode *a, const float b);
FGraphNode *pow_cd(FGraphNode *a, const double b);
// matrix multiplication, multiplication is carried out on the two inner most
// dimensions. The results of the two predecessors nodes must be available; to
// ensure that the method may execute the parameter nodes. The corresponding
// result nodes are then stored in the memory pointed to by a and b.
FGraphNode *matmul(FGraphNode **a, FGraphNode **b);
// flattens the GraphNode data to a 1 dimensional tensor, no additional data is
// allocated besides the new node
FGraphNode *flatten(FGraphNode *a);
// flattens a specific dimension of the tensor, no additional data is allocated
// besides the new node
FGraphNode *flatten_dimension(FGraphNode *a, int dimension);
#ifdef __cplusplus
}
// no c++ bindings, but function overloading for c++ header
inline FGraphNode *add(FGraphNode *a, FGraphNode *b) { return add_g(a, b); }
inline FGraphNode *add(FGraphNode *a, const int b) { return add_ci(a, b); }
inline FGraphNode *add(FGraphNode *a, const long b) { return add_cl(a, b); }
inline FGraphNode *add(FGraphNode *a, const float b) { return add_cf(a, b); }
inline FGraphNode *add(FGraphNode *a, const double b) { return add_cd(a, b); }

inline FGraphNode *sub(FGraphNode *a, FGraphNode *b) { return sub_g(a, b); }
inline FGraphNode *sub(FGraphNode *a, const int b) { return sub_ci(a, b); }
inline FGraphNode *sub(FGraphNode *a, const long b) { return sub_cl(a, b); }
inline FGraphNode *sub(FGraphNode *a, const float b) { return sub_cf(a, b); }
inline FGraphNode *sub(FGraphNode *a, const double b) { return sub_cd(a, b); }

inline FGraphNode *mul(FGraphNode *a, FGraphNode *b) { return mul_g(a, b); }
inline FGraphNode *mul(FGraphNode *a, const int b) { return mul_ci(a, b); }
inline FGraphNode *mul(FGraphNode *a, const long b) { return mul_cl(a, b); }
inline FGraphNode *mul(FGraphNode *a, const float b) { return mul_cf(a, b); }
inline FGraphNode *mul(FGraphNode *a, const double b) { return mul_cd(a, b); }

inline FGraphNode *div(FGraphNode *a, FGraphNode *b) { return div_g(a, b); }
inline FGraphNode *div(FGraphNode *a, const int b) { return div_ci(a, b); }
inline FGraphNode *div(FGraphNode *a, const long b) { return div_cl(a, b); }
inline FGraphNode *div(FGraphNode *a, const float b) { return div_cf(a, b); }
inline FGraphNode *div(FGraphNode *a, const double b) { return div_cd(a, b); }

inline FGraphNode *pow(FGraphNode *a, FGraphNode *b) { return pow_g(a, b); }
inline FGraphNode *pow(FGraphNode *a, const int b) { return pow_ci(a, b); }
inline FGraphNode *pow(FGraphNode *a, const long b) { return pow_cl(a, b); }
inline FGraphNode *pow(FGraphNode *a, const float b) { return pow_cf(a, b); }
inline FGraphNode *pow(FGraphNode *a, const double b) { return pow_cd(a, b); }

inline FGraphNode *flatten(FGraphNode *a, int dimension) {
  return flatten_dimension(a, dimension);
}
#endif
#endif
