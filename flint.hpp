#ifndef FLINT_H
#define FLINT_H
#include "src/logger.hpp"
/*
  This is the basic header file and implementation of Flint, to be compatible
  with as many languages as possible. Though it is written in C++ for object
  orientation it is as close to C as possible to guarantee compatibility,
  nevertheless it is NOT compatible to C.

  Flints Execution structure represents an AST, so that each Graph may be
  compiled to a specific OpenCL program
*/
namespace FlintBackend {
enum Type { FLOAT32, FLOAT64, INT32, INT64 };
struct Operation {
  // an Operation always is linked to the resulting tensor
  unsigned int id;
};
// each Graph Node represents one Operation with multiple ingoing edges and one
// out going edge
struct GraphNode {
  int num_predecessor;
  GraphNode *predecessors, *successor;
  Operation *operation; // the operation represented by this graph node
};
// Operations
// Store data in the tensor, always an Entry (source) of the graph
struct Store : public Operation {
  Type data_type;
  void *data;
  int num_entries;
};
// Load data from the tensor, always the Exit (sink) of the graph
struct Load : public Operation {};
// Addition of two tensors
struct Add : public Operation {};
// Subtraction of two tensors
struct Sub : public Operation {};
// Multiplication of two tensors
struct Mul : public Operation {};
// Division of two tensors
struct Div : public Operation {};
// A single-value constant (does not have to be loaded as a tensor)
template <typename T> struct Const : public Operation {
  T value; // has to be one of Type
};

// functions
// creates a Graph with a single store instruction, the data is _during
// execution_ copied to intern memory, till then the pointer has to point to
// valid data.
GraphNode *createGraph(void *data, int num_entries, Type data_type);
// frees all allocated data from the graph and the nodes that are reachable
void freeGraph(GraphNode *graph);
GraphNode *add(GraphNode *a, GraphNode *b);
GraphNode *sub(GraphNode *a, GraphNode *b);
GraphNode *div(GraphNode *a, GraphNode *b);
GraphNode *mul(GraphNode *a, GraphNode *b);
// adds the constant value to each entry in a
template <typename T> GraphNode *add(GraphNode *a, T b);
// subtracts the constant value from each entry in a
template <typename T> GraphNode *sub(GraphNode *a, T b);
// divides each entry in a by the constant value
template <typename T> GraphNode *div(GraphNode *a, T b);
// multiplicates the constant value with each entry in a
template <typename T> GraphNode *mul(GraphNode *a, T b);

}; // namespace FlintBackend
#endif
