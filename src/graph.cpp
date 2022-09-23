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

#include "../flint.h"
#include "logger.hpp"
#include "utils.hpp"
#include <cstring>
#include <list>
#include <stdlib.h>
#include <unordered_set>
#include <vector>
// INTERFACE METHODS
FGraphNode *fExecuteGraph(FGraphNode *node) {
  // TODO
  return fExecuteGraph_gpu(node);
}
void flintCleanup() {
  flintCleanup_cpu();
  flintCleanup_gpu();
}
void flintInit(int cpu, int gpu) {
  log(VERBOSE, "Initializing Flint");
  if (cpu)
    flintInit_cpu();
  if (gpu)
    flintInit_gpu();
}
// GRAPH METHODS
FGraphNode *fCreateGraph(const void *data, const int num_entries,
                         const FType data_type, const size_t *shape,
                         const int dimensions) {
  FGraphNode *gn = new FGraphNode();
  gn->reference_counter = 0;
  FOperation *op = new FOperation();
  FStore *store = new FStore();
  op->dimensions = dimensions;
  op->shape = safe_mal<size_t>(dimensions);
  std::memcpy((void *)op->shape, (void *)shape, dimensions * sizeof(size_t));
  op->additional_data = (void *)store;
  op->op_type = STORE;
  size_t byte_size = num_entries;
  switch (data_type) {
  case INT32:
    store->data = safe_mal<int>(num_entries);
    byte_size *= sizeof(int);
    break;
  case INT64:
    store->data = safe_mal<long>(num_entries);
    byte_size *= sizeof(long);
    break;
  case FLOAT32:
    store->data = safe_mal<float>(num_entries);
    byte_size *= sizeof(float);
    break;
  case FLOAT64:
    store->data = safe_mal<long>(num_entries);
    byte_size *= sizeof(double);
    break;
  }
  memcpy(store->data, data, byte_size);
  store->num_entries = num_entries;
  op->data_type = data_type;
  gn->operation = op;
  gn->num_predecessor = 0;
  gn->predecessors = NULL;
  return gn;
}
// frees all allocated data from the graph and the nodes that are reachable
void fFreeGraph(FGraphNode *graph) {
  std::unordered_set<FGraphNode *>
      all; // all which are in the queue and were visited
  std::list<FGraphNode *> wq;
  all.insert(graph);
  wq.push_back(graph);
  while (!wq.empty()) {
    FGraphNode *gn = wq.front();
    wq.pop_front();
    if (gn->reference_counter != 0) {
      continue;
    }
    for (int i = 0; i < gn->num_predecessor; i++) {
      if (gn->predecessors[i] &&
          --(gn->predecessors[i]->reference_counter) == 0 &&
          all.find(gn->predecessors[i]) == all.end()) {
        wq.push_back(gn->predecessors[i]);
        all.insert(gn->predecessors[i]);
      }
    }
    if (gn->predecessors != NULL && gn->num_predecessor != 0)
      free(gn->predecessors);
    if (gn->operation != NULL) {
      if (gn->operation->shape)
        free(gn->operation->shape);
      if (gn->operation->additional_data)
        switch (gn->operation->op_type) {
        case RESULTDATA: {
          FResultData *rd = (FResultData *)gn->operation->additional_data;
          if (rd->data)
            free(rd->data);
          if (rd->mem_id)
            clReleaseMemObject(rd->mem_id);
          delete rd;
        } break;
        case STORE: {
          FStore *st = (FStore *)gn->operation->additional_data;
          free(st->data);
          if (st->mem_id)
            clReleaseMemObject(st->mem_id);
          delete st;
        } break;
        case CONST: {
          FConst *c = (FConst *)gn->operation->additional_data;
          free(c->value);
          delete c;
        } break;
        default:
          break;
        }
      delete gn->operation;
    }
    delete gn;
  }
}
// function to add nodes to the graph i.e. operations
static FGraphNode *addNode(FOperation *op, std::vector<FGraphNode *> pre) {
  if (!op) {
    log(WARNING, "You are adding a node with a NULL operation, this is not "
                 "correct behaviour!");
  }
  FGraphNode *foo = new FGraphNode();
  foo->reference_counter = 0;
  foo->operation = op;
  foo->num_predecessor = pre.size();
  foo->predecessors =
      pre.size() == 0 ? NULL : safe_mal<FGraphNode *>(pre.size());
  for (size_t i = 0; i < pre.size(); i++) {
    foo->predecessors[i] = pre[i];
    pre[i]->reference_counter++;
  }
  return foo;
}
FGraphNode *fCopyGraph(const FGraphNode *node) {
  FGraphNode *foo = new FGraphNode();
  // predecessors
  foo->num_predecessor = node->num_predecessor;
  if (foo->num_predecessor) {
    foo->predecessors = safe_mal<FGraphNode *>(foo->num_predecessor);
    for (int i = 0; i < foo->num_predecessor; i++) {
      foo->predecessors[i] = node->predecessors[i];
      node->predecessors[i]->reference_counter++;
    }
  }

  foo->reference_counter =
      0; // is not copied since it is not yet referenced in contrast to node
  FOperation *op = new FOperation();
  foo->operation = op;
  op->data_type = node->operation->data_type;
  op->op_type = node->operation->op_type;
  op->dimensions = node->operation->dimensions;
  // shape
  if (op->dimensions) {
    op->shape = safe_mal<size_t>(op->dimensions);
    std::memcpy(op->shape, node->operation->shape,
                op->dimensions * sizeof(size_t));
  }
  // additional data
  if (node->operation->additional_data) {
    void **data = nullptr;
    void *src = nullptr;
    size_t num_entries = 0;
    switch (op->op_type) {
    case RESULTDATA: {
      FResultData *ord = (FResultData *)node->operation->additional_data;
      FResultData *crd = new FResultData();
      op->additional_data = (void *)crd;
      crd->mem_id = nullptr;
      crd->num_entries = ord->num_entries;
      num_entries = crd->num_entries;
      src = ord->data;
      data = &crd->data;
    } break;
    case STORE: {
      FStore *ord = (FStore *)node->operation->additional_data;
      FStore *crd = new FStore();
      op->additional_data = (void *)crd;
      crd->mem_id = nullptr;
      crd->num_entries = ord->num_entries;
      num_entries = crd->num_entries;
      src = ord->data;
      data = &crd->data;
    } break;
    case CONST: {
      FConst *ord = (FConst *)node->operation->additional_data;
      FConst *crd = new FConst();
      op->additional_data = (void *)crd;
      num_entries = 1;
      src = ord->value;
      data = &crd->value;
    } break;
    default:
      break;
    }
    if (data) {
      size_t byte_size = num_entries;
      switch (op->data_type) {
      case INT32:
        *data = safe_mal<int>(num_entries);
        byte_size *= sizeof(int);
        break;
      case INT64:
        *data = safe_mal<long>(num_entries);
        byte_size *= sizeof(long);
        break;
      case FLOAT32:
        *data = safe_mal<float>(num_entries);
        byte_size *= sizeof(float);
        break;
      case FLOAT64:
        *data = safe_mal<double>(num_entries);
        byte_size *= sizeof(double);
        break;
      }
      std::memcpy(*data, src, byte_size);
    }
  }
  return foo;
}
static inline void initShape_keep(FOperation *op, FOperation *a,
                                  FOperation *b) {
  size_t *src = nullptr;
  size_t *lower = nullptr;
  int lower_dim = -1;
  if (!b || a->dimensions >= b->dimensions) {
    op->dimensions = a->dimensions;
    src = a->shape;
    if (b) {
      lower = b->shape;
      lower_dim = b->dimensions;
    }
  } else {
    op->dimensions = b->dimensions;
    src = b->shape;
    lower = a->shape;
    lower_dim = a->dimensions;
  }
  // check shape if both are defined
  if (lower) {
    for (int i = 0; i < lower_dim; i++)
      if (src[i + (op->dimensions - lower_dim)] != lower[i])
        log(ERROR,
            "incompatible shapes of operands: " +
                vectorString(std::vector<size_t>(src, src + op->dimensions)) +
                " and " +
                vectorString(std::vector<size_t>(lower, lower + lower_dim)));
  }
  op->shape = (size_t *)malloc(sizeof(size_t) * op->dimensions);
  memcpy((void *)op->shape, src, sizeof(size_t) * op->dimensions);
  // determine type
  FType highest = INT32;
  if (a->data_type == FLOAT64 || (b && b->data_type == FLOAT64))
    highest = FLOAT64;
  else if (a->data_type == FLOAT32 || (b && b->data_type == FLOAT32))
    highest = FLOAT32;
  else if (a->data_type == INT64 || (b && b->data_type == INT64))
    highest = INT64;
  op->data_type = highest;
}
FGraphNode *fadd_g(FGraphNode *a, FGraphNode *b) {
  FOperation *op = new FOperation();
  op->additional_data = nullptr;
  op->op_type = ADD;
  initShape_keep(op, a->operation, b->operation);
  return addNode(op, {a, b});
}
FGraphNode *fsub_g(FGraphNode *a, FGraphNode *b) {
  FOperation *op = new FOperation();
  op->additional_data = nullptr;
  op->op_type = SUB;
  initShape_keep(op, a->operation, b->operation);
  return addNode(op, {a, b});
}
FGraphNode *fdiv_g(FGraphNode *a, FGraphNode *b) {
  FOperation *op = new FOperation();
  op->additional_data = nullptr;
  op->op_type = DIV;
  initShape_keep(op, a->operation, b->operation);
  return addNode(op, {a, b});
}
FGraphNode *fmul_g(FGraphNode *a, FGraphNode *b) {
  FOperation *op = new FOperation();
  op->additional_data = nullptr;
  op->op_type = MUL;
  initShape_keep(op, a->operation, b->operation);
  return addNode(op, {a, b});
}
FGraphNode *fpow_g(FGraphNode *a, FGraphNode *b) {
  // if (!(a->operation->dimensions >= b->operation->dimensions))
  //   log(ERROR, "pow(a, b) must fulfill a->dimensions >= b->dimensions");
  FOperation *op = new FOperation();
  op->additional_data = nullptr;
  op->op_type = POW;
  initShape_keep(op, a->operation, b->operation);
  return addNode(op, {a, b});
}
FGraphNode *fmin_g(FGraphNode *a, FGraphNode *b) {
  FOperation *op = new FOperation();
  op->additional_data = nullptr;
  op->op_type = MIN;
  initShape_keep(op, a->operation, b->operation);
  return addNode(op, {a, b});
}
FGraphNode *fmax_g(FGraphNode *a, FGraphNode *b) {
  FOperation *op = new FOperation();
  op->additional_data = nullptr;
  op->op_type = MAX;
  initShape_keep(op, a->operation, b->operation);
  return addNode(op, {a, b});
}
template <typename T>
static FGraphNode *addNodeWithConst(FOperation *op, FGraphNode *a, const T b) {
  FConst *cons = new FConst();
  T *cons_val = (T *)malloc(sizeof(T));
  *cons_val = b;
  cons->value = (void *)cons_val;
  FOperation *cop = new FOperation();
  cop->op_type = CONST;
  cop->additional_data = (void *)cons;
  if (typeid(T) == typeid(int))
    cop->data_type = INT32;
  else if (typeid(T) == typeid(long))
    cop->data_type = INT64;
  else if (typeid(T) == typeid(float))
    cop->data_type = FLOAT32;
  else if (typeid(T) == typeid(double))
    cop->data_type = FLOAT64;
  return addNode(op, {a, addNode(cop, {})});
}
// adds the constant value to each entry in a
template <typename T> FGraphNode *add(FGraphNode *a, const T b) {
  FOperation *op = new FOperation();
  op->additional_data = nullptr;
  op->op_type = ADD;
  initShape_keep(op, a->operation, nullptr);
  return addNodeWithConst(op, a, b);
}
FGraphNode *fadd_cd(FGraphNode *a, const double b) { return add<double>(a, b); }
FGraphNode *fadd_cf(FGraphNode *a, const float b) { return add<float>(a, b); }
FGraphNode *fadd_ci(FGraphNode *a, const int b) { return add<int>(a, b); }
FGraphNode *fadd_cl(FGraphNode *a, const long b) { return add<long>(a, b); }
// subtracts the constant value from each entry in a
template <typename T> FGraphNode *sub(FGraphNode *a, const T b) {
  FOperation *op = new FOperation();
  op->op_type = SUB;
  op->additional_data = nullptr;
  initShape_keep(op, a->operation, nullptr);
  return addNodeWithConst(op, a, b);
}
FGraphNode *fsub_cd(FGraphNode *a, const double b) { return sub<double>(a, b); }
FGraphNode *fsub_cf(FGraphNode *a, const float b) { return sub<float>(a, b); }
FGraphNode *fsub_ci(FGraphNode *a, const int b) { return sub<int>(a, b); }
FGraphNode *fsub_cl(FGraphNode *a, const long b) { return sub<long>(a, b); }
// divides each entry in a by the constant value
template <typename T> FGraphNode *div(FGraphNode *a, const T b) {
  FOperation *op = new FOperation();
  op->additional_data = nullptr;
  op->op_type = DIV;
  initShape_keep(op, a->operation, nullptr);
  return addNodeWithConst(op, a, b);
}
FGraphNode *fdiv_cd(FGraphNode *a, const double b) { return div<double>(a, b); }
FGraphNode *fdiv_cf(FGraphNode *a, const float b) { return div<float>(a, b); }
FGraphNode *fdiv_ci(FGraphNode *a, const int b) { return div<int>(a, b); }
FGraphNode *fdiv_cl(FGraphNode *a, const long b) { return div<long>(a, b); }
// multiplicates the constant value with each entry in a
template <typename T> FGraphNode *mul(FGraphNode *a, const T b) {
  FOperation *op = new FOperation();
  op->additional_data = nullptr;
  op->op_type = MUL;
  initShape_keep(op, a->operation, nullptr);
  return addNodeWithConst(op, a, b);
}
FGraphNode *fmul_cd(FGraphNode *a, const double b) { return mul<double>(a, b); }
FGraphNode *fmul_cf(FGraphNode *a, const float b) { return mul<float>(a, b); }
FGraphNode *fmul_ci(FGraphNode *a, const int b) { return mul<int>(a, b); }
FGraphNode *fmul_cl(FGraphNode *a, const long b) { return mul<long>(a, b); }
// takes the power of each element in a to b
template <typename T> FGraphNode *pow(FGraphNode *a, const T b) {
  FOperation *op = new FOperation();
  op->additional_data = nullptr;
  op->op_type = POW;
  initShape_keep(op, a->operation, nullptr);
  return addNodeWithConst(op, a, b);
}
FGraphNode *fpow_cd(FGraphNode *a, const double b) { return pow<double>(a, b); }
FGraphNode *fpow_cf(FGraphNode *a, const float b) { return pow<float>(a, b); }
FGraphNode *fpow_ci(FGraphNode *a, const int b) { return pow<int>(a, b); }
FGraphNode *fpow_cl(FGraphNode *a, const long b) { return pow<long>(a, b); }

template <typename T> FGraphNode *min(FGraphNode *a, const T b) {
  FOperation *op = new FOperation();
  op->additional_data = nullptr;
  op->op_type = MIN;
  initShape_keep(op, a->operation, nullptr);
  return addNodeWithConst(op, a, b);
}
FGraphNode *fmin_ci(FGraphNode *a, const int b) { return min(a, b); }
FGraphNode *fmin_cl(FGraphNode *a, const long b) { return min(a, b); }
FGraphNode *fmin_cf(FGraphNode *a, const float b) { return min(a, b); }
FGraphNode *fmin_cd(FGraphNode *a, const double b) { return min(a, b); }

template <typename T> FGraphNode *max(FGraphNode *a, const T b) {
  FOperation *op = new FOperation();
  op->additional_data = nullptr;
  op->op_type = MAX;
  initShape_keep(op, a->operation, nullptr);
  return addNodeWithConst(op, a, b);
}
FGraphNode *fmax_ci(FGraphNode *a, const int b) { return max(a, b); }
FGraphNode *fmax_cl(FGraphNode *a, const long b) { return max(a, b); }
FGraphNode *fmax_cf(FGraphNode *a, const float b) { return max(a, b); }
FGraphNode *fmax_cd(FGraphNode *a, const double b) { return max(a, b); }

FGraphNode *fflatten(FGraphNode *a) {
  FOperation *op = new FOperation();
  op->additional_data = nullptr;
  op->op_type = FLATTEN;
  op->dimensions = 1;
  op->shape = safe_mal<size_t>(1);
  const FOperation *prev_op = a->operation;
  size_t total_size = 1;
  for (int i = 0; i < prev_op->dimensions; i++)
    total_size *= prev_op->shape[i];
  op->shape[0] = total_size;
  op->additional_data = prev_op->additional_data;
  op->data_type = prev_op->data_type;
  return addNode(op, {a});
}
FGraphNode *fflatten_dimension(FGraphNode *a, const int dimension) {
  if (dimension == 0)
    log(ERROR, "Flattening the first dimension of a tensor is not possible!");

  FOperation *prev_op = a->operation;
  size_t new_prevdim_size =
      prev_op->shape[dimension - 1] * prev_op->shape[dimension];
  FOperation *op = new FOperation();
  op->additional_data = nullptr;
  op->op_type = FLATTEN;
  op->dimensions = prev_op->dimensions - 1;
  op->shape = safe_mal<size_t>(prev_op->dimensions - 1);
  // copy into shape
  memcpy(op->shape, prev_op->shape, sizeof(size_t) * dimension);
  memcpy(op->shape + dimension, prev_op->shape + (dimension + 1),
         sizeof(size_t) * (prev_op->dimensions - dimension - 1));
  op->shape[dimension - 1] = new_prevdim_size;

  op->additional_data = prev_op->additional_data;
  op->data_type = prev_op->data_type;
  return addNode(op, {a});
}
FGraphNode *freduce_sum(FGraphNode *a, const int dimension) {
  // TODO
}
FGraphNode *fmatmul(FGraphNode **a, FGraphNode **b) {
  FGraphNode *x = *a;
  FGraphNode *y = *b;
  if (x->operation->op_type != STORE && x->operation->op_type != RESULTDATA) {
    x = fExecuteGraph(x);
    *a = x;
  }
  if (y->operation->op_type != STORE && y->operation->op_type != RESULTDATA) {
    y = fExecuteGraph(y);
    *b = y;
  }
  FOperation *ao = x->operation;
  FOperation *bo = y->operation;

  if (ao->dimensions < 2 || bo->dimensions < 2)
    log(ERROR,
        "Dimensions of operands of matrix multiplications must be at least 2!");
  size_t l = ao->shape[ao->dimensions - 2];
  size_t m = ao->shape[ao->dimensions - 1];
  size_t mb = bo->shape[bo->dimensions - 2];
  size_t n = bo->shape[bo->dimensions - 1];
  if (m != mb)
    log(ERROR,
        "Incompatible Shapes for matrix multiplications: " +
            vectorString(std::vector(ao->shape, ao->shape + ao->dimensions)) +
            " and " +
            vectorString(std::vector(bo->shape, bo->shape + bo->dimensions)));
  FOperation *res = new FOperation();
  res->dimensions = std::max(ao->dimensions, bo->dimensions);
  res->shape = safe_mal<size_t>(res->dimensions);
  if (res->dimensions > 2)
    memcpy(res->shape, (ao->dimensions >= bo->dimensions ? ao : bo)->shape,
           sizeof(size_t) * (res->dimensions - 2));
  res->shape[res->dimensions - 2] = l;
  res->shape[res->dimensions - 1] = n;
  res->data_type =
      ao->data_type > bo->data_type ? ao->data_type : bo->data_type;
  res->op_type = MATMUL;

  FGraphNode *node = new FGraphNode();
  node->operation = res;
  node->num_predecessor = 2;
  node->predecessors = safe_mal<FGraphNode *>(2);
  node->predecessors[0] = x;
  node->predecessors[1] = y;
  x->reference_counter++;
  y->reference_counter++;
  node->reference_counter = 0;
  return node;
}
FGraphNode *freshape(FGraphNode *a, size_t *newshape, int dimensions) {
  FGraphNode *node = new FGraphNode();
  node->operation = new FOperation();
  node->operation->shape = safe_mal<size_t>(dimensions);
  std::memcpy(node->operation->shape, newshape, dimensions * sizeof(size_t));
  node->operation->data_type = a->operation->data_type;
  node->operation->op_type = RESHAPE;
  node->operation->dimensions = dimensions;
  node->num_predecessor = 1;
  node->predecessors = safe_mal<FGraphNode *>(1);
  node->predecessors[0] = a;
  node->reference_counter = 0;
  a->reference_counter++;
  return node;
}
FGraphNode *fconvert(FGraphNode *a, FType newtype) {
  FGraphNode *foo = new FGraphNode();
  foo->reference_counter = 0;
  foo->num_predecessor = 1;
  foo->predecessors = safe_mal<FGraphNode *>(1);
  foo->predecessors[0] = a;
  a->reference_counter++;
  foo->operation = new FOperation();
  foo->operation->data_type = newtype;
  foo->operation->dimensions = a->operation->dimensions;
  foo->operation->shape = safe_mal<size_t>(a->operation->dimensions);
  memcpy(foo->operation->shape, a->operation->shape,
         sizeof(size_t) * a->operation->dimensions);
  foo->operation->op_type = CONVERSION;
  return foo;
}
