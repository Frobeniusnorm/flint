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
#define CL_TARGET_OPENCL_VERSION 200
#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif
#ifdef __cplusplus
extern "C" {
#endif
/* \file flint.h
  \brief This is the basic header file and implementation of Flint, written in C
  to be as compatile as possible

  Flints Execution structure represents an AST, so that each Graph may be
  compiled to a specific OpenCL program
*/
#define FLINT_BACKEND_ONLY_CPU 1
#define FLINT_BACKEND_ONLY_GPU 2
#define FLINT_BACKEND_BOTH 3
/** Initializes the cpu and the gpu backends. These functions are already
 * implicitly called by the execution functions if necessary. The method allows
 * disabling of the gpu backend (by passing `FLINT_BACKEND_ONLY_CPU`), disabling
 * of the cpu backend (by passing `FLINT_BACKEND_BOTH`), initializing both
 * backends explicitly (by passing `FLINT_BACKEND_BOTH`, which is recommended,
 * since Flint is then allowed to choose the framework with heuristics). Only
 * use those functions if you...
 * - ...want to explicitly decide where and when the initialization should take
 *      place
 * - ...want to only start one backend */
void flintInit(int backends);
/** Don't call this function explicitly if you intent to use Flint normally. Use
 * `flintInit` */
void flintInit_cpu();
/** Don't call this function explicitly if you intent to use Flint normally. Use
 * `flintInit` */
void flintInit_gpu();

/** Deallocates any resourced allocated by the corresponding backends.
This method calls the other two (following) which are only executed if the
framework was initialized, else they do nothing. */
void flintCleanup();
/** Deallocates any resourced allocated by the cpu backend, if it was
 * initialized, else it does nothing. */
void flintCleanup_cpu();
/** Deallocates any resourced allocated by the gpu backend, if it was
 * initialized, else it does nothing. */
void flintCleanup_gpu();
/** Sets the logging level of the framework. Adjust this for debugging purposes,
 * or if you release software in which Flint is contained.
 * See also: `flogging`, `FLogType`
 *
 * Levels:
 * - 0: No logging
 * - 1: Only `F_ERROR`
 * - 2: Logging level `F_WARNING` (should be used for production)
 * - 3: Logging level `F_INFO` (for developement)
 * - 4: Logging level `F_VERBOSE` (for library developement)
 * - 5: Logging level `F_DEBUG` (when a bug in the library has been found)
 */
void fSetLoggingLevel(int);

/**
 * See also: `flogging`, `FLogType`
 * - `F_DEBUG` (only internal debugging informations of the framework),
 * - `F_VERBOSE` (verbose information, may be helpful to users of the
 *    library),
 * - `F_INFO` (informational data of the framework, e.g. which
 *    graphics card has been chosen),
 * - `F_ERROR` (unrecoverable errors,
 *    generated by function calls to the framework, raises a exception
 *    everytime),
 * - `F_WARNING` (probably unwanted behaviour or undefined behaviour caused
 *    by missuse of functions).
 */
enum FLogType { F_NO_LOGGING, F_ERROR, F_WARNING, F_INFO, F_VERBOSE, F_DEBUG };

/** Logs a NULL terminated string with the given logging level.
 * See also: `fSetLoggingLevel` */
void flogging(FLogType type, const char *msg);
/** All graph nodes that represent actual operations are after this call
 * executed eagerly, i.e. they are executed during graph construction. */
void fEnableEagerExecution();
/** Disable eager execution, i.e. the graph is constructed without execution of
 * the nodes until a operation makes the execution of a parent graph necessary
 * or the user calls `fExecuteGraph`. */
void fDisableEagerExecution();
/** Returns 1 if eager execution has been enabled, else 0 */
int fIsEagerExecution();
/** The 4 allowed data types:
 * - `F_INT32`(integer, 32bit)
 * - `F_INT64`(integer, 64bit)
 * - `F_FLOAT32` (floating point, 32bit)
 * - `F_FLOAT64` (floating point, 64bit)
 */
enum FType { F_INT32, F_INT64, F_FLOAT32, F_FLOAT64 };
// TODO generation functions for constants, indexing, random
enum FOperationType {
  FSTORE,
  FADD,
  FSUB,
  FMUL,
  FDIV,
  FPOW,
  FNEG,
  FLOG,
  FSIGN,
  FEVEN,
  FLOG2,
  FLOG10,
  FSIN,
  FCOS,
  FTAN,
  FASIN,
  FACOS,
  FATAN,
  FSQRT,
  FLATTEN,
  FMATMUL,
  FCONVERSION,
  FRESHAPE,
  FMIN,
  FMAX,
  FREDUCE_SUM,
  FREDUCE_MUL,
  FSLICE,
  FABS,
  FREPEAT,
  FTRANSPOSE,
  FEXTEND,
  FLESS,
  FEQUAL,
  FGREATER,
  FCONVOLVE,
  FSLIDE,
  FGRADIENT_CONVOLVE, // only for internal use!
  FNUM_OPERATION_TYPES
};
/**
 * Describes one operation. An operation always has a shape, described by
 * `FOperation.shape` which is an array of size
 * `FOperation.dimensions` with each entry denoting the size of the
 * corresponding dimension. `FOperation.op_type` denotes the type of
 * operation, `FOperation.data_type` the type of the underlying data,
 * `FOperation.additional_data` is operation specific.*/
struct FOperation {
  // shape of the data after execution
  int dimensions;
  size_t *shape;
  // type of operation, to enable switch cases and avoid v-table lookups
  FOperationType op_type;
  // datatype of result
  FType data_type;
  void *additional_data;
};
/** Stores the resulting data after an execution of `fExecuteGraph` (or implicit
 * execution). The data can be found in `FResultData.data`, the datatype in
 * `FOperation.data_type` of the corresponding `FGraphNode`.
 * The number of entries (not number of bytes) is stored in
 * `FResultData.num_entries`. The data may be consistently modified
 * if...
 * - ...the data size is changed, num_entries is equivalently updated and
 *      `realloc` is used and ...
 * - ...the data was not already loaded to the gpu (i.e. the result must be the
 *        return value of `fExecuteGraph_cpu`)
 */
struct FResultData {
  // link to gpu data
  cl_mem mem_id = nullptr;
  void *data;
  size_t num_entries;
};
/** Describes one node in the Graph. Stores the corresponding operation in
 * `FGraphNode.operation`, an array of predecessors (the arguments of
 * the operation) in `FGraphNode.predecessors`, its size in
 * `FGraphNode.num_predecessor` and the reference counter in
 * `FGraphNode.reference_counter`. Do not modify any parameter by yourself,
 * since the framework manages them, but you can read the data and structure
 * from them. The nodes are allocated by the operation functions, they and their
 * members should neither be manually created, edited or freed except by the
 * corresponding flint methods. */
struct FGraphNode {
  int num_predecessor;
  FGraphNode **predecessors;
  FOperation *operation;    // the operation represented by this graph node
  size_t reference_counter; // for garbage collection in free graph
  FResultData *result_data; // to store computational result
  void *gradient_data;      // to store a list of present variables that are
                            // currently watched in the graph
};
/** Result of an call to `fCreateGraph`, see `FResultData`.
 * Data of this Operation may not be changed manually when using a GPU Backend.
 */
struct FStore {
  // link to gpu data
  cl_mem mem_id = nullptr;
  void *data;
  size_t num_entries;
};
// TODO dirty bit to may keep store data

// A single-value constant (does not have to be loaded as a tensor)
struct FConst {
  void *value; // has to be one of Type
};
// range instructions
struct FSlice {
  long *start;
  long *end;
  long *step;
};
struct FExtend {
  size_t *start;
  long *step;
};
// functions

/**
 * - `data`: pointer to the flattened data array that should be loaded into
 *   the node
 * - `num_entries`: the number of elements (NOT BYTES!) that should
 *   be loaded
 * - `data_type`: the datatype of `data`
 * - `shape`: an array of size `dimensions`, each entry describing the
 *   size of the corresponding dimension. Make sure, `data` is at least as
 *   large as the product of all entries in `shape`
 * - `dimensions`: the number of dimensions
 *
 * Creates a Graph with a single store instruction, the data is
 * copied to intern memory, so after return of the function, `data` and `shape`
 * may be deleted. */
FGraphNode *fCreateGraph(const void *data, const int num_entries,
                         const FType data_type, const size_t *shape,
                         const int dimensions);

/** Creates a tensor that contains the single given values in all entries
 *
 * - `value`: the value this tensor should consist of
 * - `shape`: an array of size `dimensions`, each entry describing the size
 *    of the corresponding dimension.
 * - `dimensions`: the number of dimensions
 */
FGraphNode *fconstant_i(const int value, const size_t *shape,
                        const int dimensions);
/** Creates a tensor that contains the single given values in all entries
 *
 * - `value`: the value this tensor should consist of
 * - `shape`: an array of size `dimensions`, each entry describing the size
 *    of the corresponding dimension.
 * - `dimensions`: the number of dimensions
 */
FGraphNode *fconstant_l(const long value, const size_t *shape,
                        const int dimensions);
/** Creates a tensor that contains the single given values in all entries
 *
 * - `value`: the value this tensor should consist of
 * - `shape`: an array of size `dimensions`, each entry describing the size
 *    of the corresponding dimension.
 * - `dimensions`: the number of dimensions
 */
FGraphNode *fconstant_f(const float value, const size_t *shape,
                        const int dimensions);
/** Creates a tensor that contains the single given values in all entries
 *
 * - `value`: the value this tensor should consist of
 * - `shape`: an array of size `dimensions`, each entry describing the size
 *    of the corresponding dimension.
 * - `dimensions`: the number of dimensions
 */
FGraphNode *fconstant_d(const double value, const size_t *shape,
                        const int dimensions);
/** Decrements `FGraphNode.reference_counter` of `graph` (for reference
 * counting) and deallocates the node and its corresponding data, if the counter
 * becomes 0. If the node is deallocated, the same process is repeated with its
 * predecessors. So you can safely connect nodes multiple times and have only to
 * free the leaf nodes (i.e. the results), without caring about cross-reference,
 * since those are handled by the reference counting system.*/
void fFreeGraph(FGraphNode *graph);
/** Copies the graph node, the corresponding operation and additional data and
 * the predecessors (their `FGraphNode.reference_counter` is
 * incremented) */
FGraphNode *fCopyGraph(const FGraphNode *graph);
/** Executes the graph node operations from all yet to be executed predecessors
 * to `node` and returns a node with a `FResultData` operation in
 * which the resulting data is stored. If the graph is executed by the GPU
 * backend, a opencl kernel containing all selected operations (the nodes
 * operation and those indirect parent operations which were not yet
 * executed) are compiled and executed. The kernels are cashed, so it improves
 * the performance of a program if the same graph-structures are reused (not
 * necessary the same nodes, but the same combination of nodes), since then the
 * backend can reuse already compiled kernels. If the CPU backend is chosen, it
 * does not matter, since every operation is executed independently.*/
FGraphNode *fExecuteGraph(FGraphNode *node);
/** Executes the graph node operations from all yet to be executed predecessors
 * to `node` and returns a node with a `FResultData` operation in
 * which the resulting data is stored. */
FGraphNode *fExecuteGraph_cpu(FGraphNode *node);
/** Executes the graph node operations from all yet to be executed predecessors
 * to `node` and returns a node with a `FResultData` operation in
 * which the resulting data is stored. For the GPU
 * backend, a opencl kernel containing all selected operations (the nodes
 * operation and those indirect parent operations which were not yet executed)
 * are compiled and executed. The kernels are cashed, so it improves the
 * performance of a program if the same graph-structures are reused (not
 * necessary the same nodes, but the same combination of nodes), since then the
 * backend can reuse already compiled kernels. */
FGraphNode *fExecuteGraph_gpu(FGraphNode *node);
/** Executes the graph node operations from all yet to be executed predecessors
 * to `node` and returns a node with a `FResultData` operation in
 * which the resulting data is stored. */
FGraphNode *fExecuteGraph_cpu_eagerly(FGraphNode *node);
/** Executes the graph node operations from all yet to be executed predecessors
 * to `node` and returns a node with a `FResultData` operation in
 * which the resulting data is stored. For the GPU
 * backend, a opencl kernel containing all selected operations (the nodes
 * operation and those indirect parent operations which were not yet executed)
 * are compiled and executed. The kernels are cashed, so it improves the
 * performance of a program if the same graph-structures are reused (not
 * necessary the same nodes, but the same combination of nodes), since then the
 * backend can reuse already compiled kernels. */
FGraphNode *fExecuteGraph_gpu_eagerly(FGraphNode *node);
//  gradient calculation
/** Calculates the overall gradient of an output node to a variable.
 * The variable must be marked as a gradient variable, see
 * `fMarkGradientVariable`.
 *
 * - `outputfct`: the Node which represents the chain of functions of which
 *    the gradient is to be computed.
 * - `dx`: the variable for which outputfct is derived for
 */
FGraphNode *fCalculateGradient(FGraphNode *outputfct, const FGraphNode *dx);
/** Marks this node as a node for which a gradient might be calculated later.
 * It is only possible to calculate the gradient for this node (as a derivative)
 * in operations that occur AFTER a call to this method (all subsequent
 * operations will have a remark that enlists this node as a possible gradient
 * variable, to enable less memory usage and faster gradient calculation).
 */
void fMarkGradientVariable(FGraphNode *node);
/** Removes the gradient mark (ans subsequent memory overhead) for this node.
 * After a call to this method no subsequent gradient calculations with this
 * node as a derivative will be possible.
 */
void fUnmarkGradientVariable(FGraphNode *node);
/** Optimizes memory by freeing all parental data (operand nodes of the
 * operation of this node) and transforming this node to a storage nodes
 * if no gradient variables are present in this node and result data is
 * present (i.e. it has been executed).
 * If you call this function manually please make sure to increase the
 * `reference_counter` of the parents if you want to keep their handles,
 * since this function may decrease their counter and free them.
 * The C++ framework does this automatically.
 */
FGraphNode *fOptimizeMemory(FGraphNode *node);
//  operations
/** Serializes the data and shape of the node and returns a array of chars in
 * which the serialized data will be written. The returned array is allocated on
 * the systems heap with `malloc`, so you have to free it after you are done
 * with it. The number of bytes that the returned array has is written into the
 * memory `bytes_written` points to if it is not a nullptr. If the node doesn't
 * have result data, it is executed first.
 */
char *fserialize(FGraphNode *node, size_t *bytes_written);
/** Unserializes data generated by `fserialize`.
 * The size of the data is stored in itself, therefor no extra parameters are
 * needed. Internally calls `fCreateGraph`.
 */
FGraphNode *fdeserialize(char *data);
/** Elementwise addition of `a` and `b`, i.e. `a[i] + b[i]`. */
FGraphNode *fadd_g(FGraphNode *a, FGraphNode *b);
/** Elementwise substraction of `a` and `b`, i.e. `a[i] - b[i]`. */
FGraphNode *fsub_g(FGraphNode *a, FGraphNode *b);
/** Elementwise division of `a` and `b`, i.e. `a[i] / b[i]`. */
FGraphNode *fdiv_g(FGraphNode *a, FGraphNode *b);
/** Elementwise multiplication of `a` and `b`, i.e. `a[i] * b[i]`. */
FGraphNode *fmul_g(FGraphNode *a, FGraphNode *b);
/** Elementwise power of `a` and `b`, i.e. `pow(a[i], b[i])`. */
FGraphNode *fpow_g(FGraphNode *a, FGraphNode *b);

/** Elementwise addition of a and b, i.e. `a[i] + b`. */
FGraphNode *fadd_ci(FGraphNode *a, const int b);
/** Elementwise addition of a and b, i.e. `a[i] + b`. */
FGraphNode *fadd_cl(FGraphNode *a, const long b);
/** Elementwise addition of a and b, i.e. `a[i] + b`. */
FGraphNode *fadd_cf(FGraphNode *a, const float b);
/** Elementwise addition of a and b, i.e. `a[i] + b`. */
FGraphNode *fadd_cd(FGraphNode *a, const double b);

/** Elementwise subtraction of a and b, i.e. `a[i] - b`. */
FGraphNode *fsub_ci(FGraphNode *a, const int b);
/** Elementwise subtraction of a and b, i.e. `a[i] - b`. */
FGraphNode *fsub_cl(FGraphNode *a, const long b);
/** Elementwise subtraction of a and b, i.e. `a[i] - b`. */
FGraphNode *fsub_cf(FGraphNode *a, const float b);
/** Elementwise subtraction of a and b, i.e. `a[i] - b`. */
FGraphNode *fsub_cd(FGraphNode *a, const double b);

/** Elementwise subtraction of a and b, i.e. `a - b[i]`. */
FGraphNode *fsub_ici(const int a, FGraphNode *b);
/** Elementwise subtraction of a and b, i.e. `a - b[i]`. */
FGraphNode *fsub_icl(const long a, FGraphNode *b);
/** Elementwise subtraction of a and b, i.e. `a - b[i]`. */
FGraphNode *fsub_icf(const float a, FGraphNode *b);
/** Elementwise subtraction of a and b, i.e. `a - b[i]`. */
FGraphNode *fsub_icd(const double a, FGraphNode *b);

/** Elementwise division of a and b, i.e. `a[i] / b`. */
FGraphNode *fdiv_ci(FGraphNode *a, const int b);
/** Elementwise division of a and b, i.e. `a[i] / b`. */
FGraphNode *fdiv_cl(FGraphNode *a, const long b);
/** Elementwise division of a and b, i.e. `a[i] / b`. */
FGraphNode *fdiv_cf(FGraphNode *a, const float b);
/** Elementwise division of a and b, i.e. `a[i] / b`. */
FGraphNode *fdiv_cd(FGraphNode *a, const double b);

/** Elementwise division of a and b, i.e. `a / b[i]`. */
FGraphNode *fdiv_ici(const int a, FGraphNode *b);
/** Elementwise division of a and b, i.e. `a / b[i]`. */
FGraphNode *fdiv_icl(const long a, FGraphNode *b);
/** Elementwise division of a and b, i.e. `a / b[i]`. */
FGraphNode *fdiv_icf(const float a, FGraphNode *b);
/** Elementwise division of a and b, i.e. `a / b[i]`. */
FGraphNode *fdiv_icd(const double a, FGraphNode *b);

/** Elementwise multiplication of a and b, i.e. `a[i] * b`. */
FGraphNode *fmul_ci(FGraphNode *a, const int b);
/** Elementwise multiplication of a and b, i.e. `a[i] * b`. */
FGraphNode *fmul_cl(FGraphNode *a, const long b);
/** Elementwise multiplication of a and b, i.e. `a[i] * b`. */
FGraphNode *fmul_cf(FGraphNode *a, const float b);
/** Elementwise multiplication of a and b, i.e. `a[i] * b`. */
FGraphNode *fmul_cd(FGraphNode *a, const double b);

/** Takes the elementwise power of a to b, i.e. `pow(a[i], b)`.*/
FGraphNode *fpow_ci(FGraphNode *a, const int b);
/** Takes the elementwise power of a to b, i.e. `pow(a[i], b)`.*/
FGraphNode *fpow_cl(FGraphNode *a, const long b);
/** Takes the elementwise power of a to b, i.e. `pow(a[i], b)`.*/
FGraphNode *fpow_cf(FGraphNode *a, const float b);
/** Takes the elementwise power of a to b, i.e. `pow(a[i], b)`.*/
FGraphNode *fpow_cd(FGraphNode *a, const double b);

/** Takes the elementwise natural logarithm of `a` */
FGraphNode *flog(FGraphNode *a);
/** Takes the elementwise logarithm of `a` to the basis of 2 */
FGraphNode *flog2(FGraphNode *a);
/** Takes the elementwise logarithm of `a` to the basis of 10 */
FGraphNode *flog10(FGraphNode *a);
/** Takes the elementwise sinus of a */
FGraphNode *fsin(FGraphNode *a);
/** Takes the elementwise square root of a */
FGraphNode *fsqrt_g(FGraphNode *a);
/** Takes the elementwise cosinus of a */
FGraphNode *fcos(FGraphNode *a);
/** Takes the elementwise tangents of a */
FGraphNode *ftan(FGraphNode *a);
/** Takes the elementwise inverse sinus of a */
FGraphNode *fasin(FGraphNode *a);
/** Takes the elementwise inverse cosinus of a */
FGraphNode *facos(FGraphNode *a);
/** Takes the elementwise inverse tangents of a */
FGraphNode *fatan(FGraphNode *a);
/** Negates the elements of `a`, i.e. `-a[i]` */
FGraphNode *fneg(FGraphNode *a);
/** Returns a `F_INT32` tensor x with the shape of a with `x[i] = 1` if `a[i] >=
 * 0` else `x[i] = -1`. `a` needs to have a integer data type.*/
FGraphNode *fsign(FGraphNode *a);
/** Returns a `F_INT32` tensor `x` with the shape of `a` with `x[i] = 1` if
 * `a[i] % 2 = 0` else `x[i] = 0`.
 */
FGraphNode *feven(FGraphNode *a);
/** Compares two tensors elementwise by `a < b` and returns a 0,1 `F_INT32`
 * Tensor. `0` denotes that `a >= b`, `1` that `a < b`. */
FGraphNode *fless_g(FGraphNode *a, FGraphNode *b);
/** Compares two tensors elementwise by `a > b` and returns a 0,1 `F_INT32`
 * Tensor. */
FGraphNode *fgreater_g(FGraphNode *a, FGraphNode *b);
/** Compares two tensors elementwise by `a = b`` and returns a 0,1 `F_INT32`
 * Tensor. `0` denotes that `a <= b`, `1` that `a > b`.*/
FGraphNode *fequal_g(FGraphNode *a, FGraphNode *b);
/** Compares a tensor and a constant elementwise by `a < b` and returns a 0,1
 * `INT32` Tensor. See `fless_g`. */
FGraphNode *fless_ci(FGraphNode *a, const int b);
/** Compares a tensor and a constant elementwise by `a < b` and returns a 0,1
 * `INT32` Tensor. See `fless_g`. */
FGraphNode *fless_cl(FGraphNode *a, const long b);
/** Compares a tensor and a constant elementwise by `a < b` and returns a 0,1
 * `INT32` Tensor. See `fless_g`. */
FGraphNode *fless_cf(FGraphNode *a, const float b);
/** Compares a tensor and a constant elementwise by `a < b` and returns a 0,1
 * `INT32` Tensor. See `fless_g`. */
FGraphNode *fless_cd(FGraphNode *a, const double b);
/** Compares a tensor and a constant elementwise by `a > b` and returns a 0,1
 * `INT32` Tensor. See `fgreater_g`. */
FGraphNode *fgreater_ci(FGraphNode *a, const int b);
/** Compares a tensor and a constant elementwise by `a > b` and returns a 0,1
 * `INT32` Tensor. See `fgreater_g`. */
FGraphNode *fgreater_cl(FGraphNode *a, const long b);
/** Compares a tensor and a constant elementwise by `a > b` and returns a 0,1
 * `INT32` Tensor. See `fgreater_g`. */
FGraphNode *fgreater_cf(FGraphNode *a, const float b);
/** Compares a tensor and a constant elementwise by `a > b` and returns a 0,1
 * `INT32` Tensor. See `fgreater_g`. */
FGraphNode *fgreater_cd(FGraphNode *a, const double b);
/** Compares a tensor and a constant elementwise by `a = b` and returns a 0,1
 * `INT32` Tensor. See `fequal_g`. */
FGraphNode *fequal_ci(FGraphNode *a, const int b);
/** Compares a tensor and a constant elementwise by `a = b` and returns a 0,1
 * `INT32` Tensor. See `fequal_g`. */
FGraphNode *fequal_cl(FGraphNode *a, const long b);
/** Compares a tensor and a constant elementwise by `a = b` and returns a 0,1
 * `INT32` Tensor. See `fequal_g`. */
FGraphNode *fequal_cf(FGraphNode *a, const float b);
/** Compares a tensor and a constant elementwise by `a = b` and returns a 0,1
 * `INT32` Tensor. See `fequal_g`. */
FGraphNode *fequal_cd(FGraphNode *a, const double b);
/** Carries out matrix multiplication on the last two dimensions of the tensors.

 * E.g. a matrix multiplication of two tensors with shapes `(64, 32, 16)` and
 * `(16, 24)` will yield a tensor with shape `(64, 32, 24)`.
 *
 * Since for one entry of the
 * tensor multiple other previous entries are needed, the operand tensors need
 * to be executed first. Therefor the method will implicitly (or eagerly)
 * execute the two parameter nodes `a` and `b` if their data is not allready
 * present. */
FGraphNode *fmatmul(FGraphNode *a, FGraphNode *b);
/** Flattens the complete tensor to a tensor with one
dimension.
E.g.`flattened([[[3, 1, 4], [2, 1, 5]], [[0, 4, 2], [4, 7, 9]]]) = [3, 1, 4, 2,
1, 5, 0, 4, 2, 4, 7, 9]`.
*/
FGraphNode *fflatten(FGraphNode *a);
/** Flattens a tensor `a` with `n` dimensions along
`dimension`, resulting in a tensor with `n-1` dimensions.
Flattening a dimension will remove it from the shape of the tensor, therefor its
not possible to flatten the dimension 0.
A Tensor `[[[3, 1, 4], [2, 1, 5]], [[0, 4, 2], [4, 7, 9]]]` flattened
along dimension 1 will result in `[[3,1,4], [2,1,5], [0,4,2], [4,7,9]]`.
*/
FGraphNode *fflatten_dimension(FGraphNode *a, int dimension);

/** Converts the data of `a` to the type given by `newtype`*/
FGraphNode *fconvert(FGraphNode *a, FType newtype);
/** Reshapes the underlying data of the tensor to the new shape. The product of
  each dimension of the new shape must be the same as the product of the
  dimensions of the previous shape (i.e. it must describe the same number of
  entries of the tensor).*/
FGraphNode *freshape(FGraphNode *a, size_t *newshape, int dimensions);
/** Takes the minimum of two tensors element wise along the last dimension of
 * each, i.e. `a[i]` if `a[i] < b[i]` else `b[i]` */
FGraphNode *fmin_g(FGraphNode *a, FGraphNode *b);
/** Takes the minimum of two tensors element wise along the last dimension of
 * each, i.e. `a[i]` if `a[i] < b` else `b` */
FGraphNode *fmin_ci(FGraphNode *a, const int b);
/** Takes the minimum of two tensors element wise along the last dimension of
 * each, i.e. `a[i]` if `a[i] < b` else `b` */
FGraphNode *fmin_cl(FGraphNode *a, const long b);
/** Takes the minimum of two tensors element wise along the last dimension of
 * each, i.e. `a[i]` if `a[i] < b` else `b` */
FGraphNode *fmin_cf(FGraphNode *a, const float b);
/** Takes the minimum of two tensors element wise along the last dimension of
 * each, i.e. `a[i]` if `a[i] < b` else `b` */
FGraphNode *fmin_cd(FGraphNode *a, const double b);
/** Takes the maximum of two tensors element wise along the last dimension of
 * each, i.e. `a[i]` if `a[i] > b[i]` else `b[i]` */
FGraphNode *fmax_g(FGraphNode *a, FGraphNode *b);
/** Takes the maximum of two tensors element wise along the last dimension of
 * each, i.e. `a[i]` if `a[i] > b` else `b` */
FGraphNode *fmax_ci(FGraphNode *a, const int b);
/** Takes the maximum of two tensors element wise along the last dimension of
 * each, i.e. `a[i]` if `a[i] > b` else `b` */
FGraphNode *fmax_cl(FGraphNode *a, const long b);
/** Takes the maximum of two tensors element wise along the last dimension of
 * each, i.e. `a[i]` if `a[i] > b` else `b` */
FGraphNode *fmax_cf(FGraphNode *a, const float b);
/** Takes the maximum of two tensors element wise along the last dimension of
 * each, i.e. `a[i]` if `a[i] > b` else `b` */
FGraphNode *fmax_cd(FGraphNode *a, const double b);
/** Reduces one dimension of the tensor by additive folding e.g.
 *
 * `freduce_sum([[1,2,3], [4,5,6]], 0) = [5,7,9]`,
 * `freduce_sum([[1,2,3], [4,5,6]], 1) = [6,15]`
 *
 * The results of the predecessor node must be available, to
 * ensure that the method may execute the parameter node. */
FGraphNode *freduce_sum(FGraphNode *a,
                        const int dimension);
/** Reduces one dimension of the tensor by multiplicative folding e.g.
 *
 * `freduce_mul([[1,2,3], [4,5,6]], 0) = [4,10,18]`,
 * `freduce_mul([[1,2,3], [4,5,6]], 1) = [6, 120]`
 *
 * The results of the predecessor node must be available; to
 * ensure that the method may execute the parameter node.*/
FGraphNode *freduce_mul(FGraphNode *a, const int dimension);
/** Selects a slice of the tensor with a dimension wise start and end index.
 * `start` and `end` are arrays with as many entries
 * as the tensor has dimensions. They may contain negative values,
 * which are then subtracted from the end of the tensor (e.g. `-1` means the
 * element before the last element). `start` is inclusive and describes the
 * start index of the selection per dimension and `end` describes the end index
 * per dimension and is exclusive.
 */
FGraphNode *fslice(FGraphNode *a, const long *start, const long *end);
/** Selects a slice of the tensor with a dimension wise start index, end index
 * and step size. `start`, `end` and `step` are arrays with as
 * many entries as the tensor has dimensions. `start` and `end` may
 * contain negative values, which are then subtracted from the end of the tensor
 * (e.g. `-1` means the element before the last element). `start` is inclusive
 * and describes the start index of the selection per dimension and `end`
 * describes the end index per dimension and is exclusive. `step` contains the
 * per dimension step size (e.g. `2` meaning every second element will be
 * selected etc.) and may be negative as well, which reverses the traversal
 * order (the first elements are selected as the last ones). For a negative step
 * size, `start > end` must hold (for a positive of course `end > start`) for
 * each dimension.
 */
FGraphNode *fslice_step(FGraphNode *a, const long *start, const long *end,
                        const long *step);
/**
 * Creates a new tensor of zeroes with the requested shape. The original tensor
 * is embedded at the given indices.
 * - `a` original tensor which shape is to be extended
 * - `new_shape` array of new sizes per dimension. Has `dimension` number of
 *    entries
 * - `insert_at` array with indices per dimension denoting where `a` is to be
 *    placed in the resulting tensor
 */
FGraphNode *fextend(FGraphNode *a, const size_t *new_shape,
                    const size_t *insert_at);
/**
 * Creates a new tensor of zeroes with the requested shape. The original tensor
 * is embedded at the given indices.
 * - `a` original tensor which shape is to be extended,
 * - `new_shape` array of new sizes per dimension. Has `dimension` number of
 *    entries.
 * - `insert_at` array with indices per dimension denoting where `a` is to be
 *    placed in the resulting tensor. Has a value per dimension.
 * - `step_size` allows to pull apart `a`, emplacing `step_size[i]` 0 between
 *    each value of `a`. Has a value per dimension.
 */
FGraphNode *fextend_step(FGraphNode *a, const size_t *new_shape,
                         const size_t *insert_at, const long *step_size);
/** Takes the elementwise absolute value of `a`, i.e. `|a[i]|` */
FGraphNode *fabs_g(FGraphNode *a);
/** Repeats dimensions of a tensor multiple times
 *
 * - `a`: the node in which dimensions are to be repeated
 * - `repititions`: array with the same number of entries as the tensor has
 *    dimensions
 * e.g. `repeat([[0,1], [2,3]], [2, 3]) = [[0,1,0,1,0,1],
 * [2,3,2,3,2,3], [0,1,0,1,0,1], [2,3,2,3,2,3]]`
 */
FGraphNode *frepeat(FGraphNode *a, int *repititions);
/** Transposes this tensor along multiple dimensions
 *
 * - `a`: the node which should be transposed
 * - `transpositions`: an array with the same number of entries as the tensor
 *   has dimensions, which gives the perumtation of dimensions.
 * The tensor will have a resulting shape in which the size in dimension `i`
 * corresponds to the former size in dimension `transpositions[i]`.
 */
FGraphNode *ftranspose(FGraphNode *a, int *transpositions);
/** Convolves the `n`-dimensional input tensor `a` with a `n`-dimensional filter
 * kernel `kernel` and a per dimensional step size `steps` with size of `n-1`.
 * It is expected that `a` and `kernel` have the same size in their last
 * dimension (which will be completly reduced by the convolution). In all other
 * dimensions the size of `a` should be larger or equal to the size of `kernel`.
 * The `kernel` will be 'slid' over `a` in each dimension, multiplying all
 * values of `kernel` with the corresponding ones in `a` and summing them up to
 * a single value and moving the kernel further by the value given in `steps` in
 * that corresponding dimension. The implementation does not care about padding
 * (if `a` is in a dimension not divisable by the step size, the size is rounded
 * up. If in one step the kernel overlaps over the Tensor for one or more
 * dimensions, the overlapping values will be multiplied by `0`). If you want to
 * include it use `fextend` or similar.
 *
 * The resulting Tensor will therefor have a shape with dimensionality `n - 1`
 * and size of `resulting_shape[i] = a->operation->shape[i] / steps[i]`
 */
FGraphNode *fconvolve(FGraphNode *a, FGraphNode *kernel, unsigned int *steps);
/**
 * Slides `kernel` along `a`, multiplying it with the elements of `a` it is slid
 * over. For each element all multiplied values are summed up, so that the
 * result has the same shape as `kernel` (every element in the result is the
 * accumulated sum of the product of that element with all elements it was slid
 * over). `kernel` is initially placed so that the first element of `a` and
 * the first element of `kernel` overlap. It is then moved for each dimension
 * `i` by `steps[i]` elements forward, just like it would be by ´fconvolve` with
 * the difference, that everything is accumulated for the kernel instead of the
 * original node.
 * The number of steps is in contrast to `fconvolve` not limited to every
 * dimension except the last, `fslide` can slide the Tensor along all dimensions
 * (NOTE: this may change in the future).
 */
FGraphNode *fslide(FGraphNode *a, FGraphNode *kernel, unsigned int *steps);
#ifdef __cplusplus
}
// no c++ bindings, but function overloading for c++ header
inline FGraphNode *fconstant(const int value, const size_t *shape,
                             const int dimensions) {
  return fconstant_i(value, shape, dimensions);
}
inline FGraphNode *fconstant(const long value, const size_t *shape,
                             const int dimensions) {
  return fconstant_l(value, shape, dimensions);
}
inline FGraphNode *fconstant(const float value, const size_t *shape,
                             const int dimensions) {
  return fconstant_f(value, shape, dimensions);
}
inline FGraphNode *fconstant(const double value, const size_t *shape,
                             const int dimensions) {
  return fconstant_d(value, shape, dimensions);
}

inline FGraphNode *fadd(FGraphNode *a, FGraphNode *b) { return fadd_g(a, b); }
inline FGraphNode *fadd(FGraphNode *a, const int b) { return fadd_ci(a, b); }
inline FGraphNode *fadd(FGraphNode *a, const long b) { return fadd_cl(a, b); }
inline FGraphNode *fadd(FGraphNode *a, const float b) { return fadd_cf(a, b); }
inline FGraphNode *fadd(FGraphNode *a, const double b) { return fadd_cd(a, b); }

inline FGraphNode *fsub(FGraphNode *a, FGraphNode *b) { return fsub_g(a, b); }
inline FGraphNode *fsub(FGraphNode *a, const int b) { return fsub_ci(a, b); }
inline FGraphNode *fsub(FGraphNode *a, const long b) { return fsub_cl(a, b); }
inline FGraphNode *fsub(FGraphNode *a, const float b) { return fsub_cf(a, b); }
inline FGraphNode *fsub(FGraphNode *a, const double b) { return fsub_cd(a, b); }

inline FGraphNode *fmul(FGraphNode *a, FGraphNode *b) { return fmul_g(a, b); }
inline FGraphNode *fmul(FGraphNode *a, const int b) { return fmul_ci(a, b); }
inline FGraphNode *fmul(FGraphNode *a, const long b) { return fmul_cl(a, b); }
inline FGraphNode *fmul(FGraphNode *a, const float b) { return fmul_cf(a, b); }
inline FGraphNode *fmul(FGraphNode *a, const double b) { return fmul_cd(a, b); }

inline FGraphNode *fdiv(FGraphNode *a, FGraphNode *b) { return fdiv_g(a, b); }
inline FGraphNode *fdiv(FGraphNode *a, const int b) { return fdiv_ci(a, b); }
inline FGraphNode *fdiv(FGraphNode *a, const long b) { return fdiv_cl(a, b); }
inline FGraphNode *fdiv(FGraphNode *a, const float b) { return fdiv_cf(a, b); }
inline FGraphNode *fdiv(FGraphNode *a, const double b) { return fdiv_cd(a, b); }

inline FGraphNode *fdiv(const int a, FGraphNode *b) { return fdiv_ici(a, b); }
inline FGraphNode *fdiv(const long a, FGraphNode *b) { return fdiv_icl(a, b); }
inline FGraphNode *fdiv(const float a, FGraphNode *b) { return fdiv_icf(a, b); }
inline FGraphNode *fdiv(const double a, FGraphNode *b) {
  return fdiv_icd(a, b);
}

inline FGraphNode *fpow(FGraphNode *a, FGraphNode *b) { return fpow_g(a, b); }
inline FGraphNode *fpow(FGraphNode *a, const int b) { return fpow_ci(a, b); }
inline FGraphNode *fpow(FGraphNode *a, const long b) { return fpow_cl(a, b); }
inline FGraphNode *fpow(FGraphNode *a, const float b) { return fpow_cf(a, b); }
inline FGraphNode *fpow(FGraphNode *a, const double b) { return fpow_cd(a, b); }

inline FGraphNode *fmin(FGraphNode *a, FGraphNode *b) { return fmin_g(a, b); }
inline FGraphNode *fmin(FGraphNode *a, const int b) { return fmin_ci(a, b); }
inline FGraphNode *fmin(FGraphNode *a, const long b) { return fmin_cl(a, b); }
inline FGraphNode *fmin(FGraphNode *a, const float b) { return fmin_cf(a, b); }
inline FGraphNode *fmin(FGraphNode *a, const double b) { return fmin_cd(a, b); }

inline FGraphNode *fmax(FGraphNode *a, FGraphNode *b) { return fmax_g(a, b); }
inline FGraphNode *fmax(FGraphNode *a, const int b) { return fmax_ci(a, b); }
inline FGraphNode *fmax(FGraphNode *a, const long b) { return fmax_cl(a, b); }
inline FGraphNode *fmax(FGraphNode *a, const float b) { return fmax_cf(a, b); }
inline FGraphNode *fmax(FGraphNode *a, const double b) { return fmax_cd(a, b); }

inline FGraphNode *fless(FGraphNode *a, FGraphNode *b) { return fless_g(a, b); }
inline FGraphNode *fless(FGraphNode *a, const int b) { return fless_ci(a, b); }
inline FGraphNode *fless(FGraphNode *a, const long b) { return fless_cl(a, b); }
inline FGraphNode *fless(FGraphNode *a, const float b) {
  return fless_cf(a, b);
}
inline FGraphNode *fless(FGraphNode *a, const double b) {
  return fless_cd(a, b);
}

inline FGraphNode *fequal(FGraphNode *a, FGraphNode *b) {
  return fequal_g(a, b);
}
inline FGraphNode *fequal(FGraphNode *a, const int b) {
  return fequal_ci(a, b);
}
inline FGraphNode *fequal(FGraphNode *a, const long b) {
  return fequal_cl(a, b);
}
inline FGraphNode *fequal(FGraphNode *a, const float b) {
  return fequal_cf(a, b);
}
inline FGraphNode *fequal(FGraphNode *a, const double b) {
  return fequal_cd(a, b);
}

inline FGraphNode *fgreater(FGraphNode *a, FGraphNode *b) {
  return fgreater_g(a, b);
}
inline FGraphNode *fgreater(FGraphNode *a, const int b) {
  return fgreater_ci(a, b);
}
inline FGraphNode *fgreater(FGraphNode *a, const long b) {
  return fgreater_cl(a, b);
}
inline FGraphNode *fgreater(FGraphNode *a, const float b) {
  return fgreater_cf(a, b);
}
inline FGraphNode *fgreater(FGraphNode *a, const double b) {
  return fgreater_cd(a, b);
}

inline FGraphNode *fflatten(FGraphNode *a, int dimension) {
  return fflatten_dimension(a, dimension);
}
#include <string>
inline void flogging(FLogType type, std::string msg) {
  flogging(type, msg.c_str());
}
#endif
#endif
