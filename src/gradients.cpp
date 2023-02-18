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

#ifndef GRADIENTS_CPP
#define GRADIENTS_CPP
#include "../flint.h"
#include <cmath>
#include <cstring>
#include <iostream>
#include <list>
#include <math.h>
#include <ostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
// converts c++ type to flint type
template <typename T> static constexpr FType toFlintType() {
  if (std::is_same<T, int>())
    return F_INT32;
  if (std::is_same<T, long>())
    return F_INT64;
  if (std::is_same<T, float>())
    return F_FLOAT32;
  if (std::is_same<T, double>())
    return F_FLOAT64;
}
static FGraphNode *constant_tensor(double val, FType type, size_t *shape,
                                   int dimensions) {
  switch (type) {
  case F_FLOAT32:
    return fconstant_f((float)val, shape, dimensions);
  case F_INT32:
    return fconstant_i((int)val, shape, dimensions);
  case F_INT64:
    return fconstant_l((long)val, shape, dimensions);
  case F_FLOAT64:
    return fconstant_d((double)val, shape, dimensions);
  }
}
static FGraphNode *unbroadcast(FGraphNode *adjoint, const FGraphNode *node) {
  if (adjoint->operation->dimensions > node->operation->dimensions) {
    size_t diff = adjoint->operation->dimensions - node->operation->dimensions;
    FGraphNode *res = adjoint;
    for (int i = 0; i < diff; i++) {
      res = freduce_sum(res, 0);
    }
    return res;
  } else if (adjoint->operation->dimensions < node->operation->dimensions) {
    size_t diff = node->operation->dimensions - adjoint->operation->dimensions;
    std::vector<size_t> new_shape(node->operation->dimensions);
    std::vector<int> repetitions(node->operation->dimensions, 0);
    for (int i = 0; i < diff; i++) {
      new_shape[i] = 1;
      repetitions[i] = node->operation->shape[i] - 1;
    }
    for (int i = diff; i < new_shape.size(); i++)
      new_shape[i] = adjoint->operation->shape[i - diff];
    FGraphNode *res = freshape(adjoint, new_shape.data(), new_shape.size());
    res = frepeat(res, repetitions.data());
    return res;
  }
  return adjoint;
}
static FGraphNode *local_gradient(const FGraphNode *y, FGraphNode *dx,
                                  FGraphNode *prev_adj) {
  switch (y->operation->op_type) {
  case FADD:
    return (dx == y->predecessors[0] || dx == y->predecessors[1]) ? prev_adj
                                                                  : nullptr;
  case FSUB:
    if (dx == y->predecessors[0])
      return prev_adj;
    else if (dx == y->predecessors[1])
      return fneg(prev_adj);
    else
      return nullptr;
  case FMUL: {
    if (y->predecessors[0] == dx) {
      return fmul(prev_adj, y->predecessors[1]);
    } else if (y->predecessors[1] == dx) {
      return fmul(prev_adj, y->predecessors[0]);
    } else
      return nullptr;
  }
  case FDIV: {
    FGraphNode *a = y->predecessors[0];
    FGraphNode *b = y->predecessors[1];
    if (a == dx) {
      // d(a / b)/da = d(a * b^(-1))/da = b^(-1)
      return fdiv(prev_adj, b);
    } else if (b == dx) {
      // d(a / b)/db = d(a * b^(-1))/db = -a * b^(-2)
      return fneg(fdiv(fmul(prev_adj, a), fpow(b, 2.)));
    } else
      return nullptr;
  }
  case FMATMUL: {
    FGraphNode *a = y->predecessors[0];
    FGraphNode *b = y->predecessors[1];
    if (a == dx) {
      std::vector<int> perm(b->operation->dimensions);
      for (int i = 0; i < perm.size() - 2; i++)
        perm[i] = i;
      perm[perm.size() - 2] = perm.size() - 1;
      perm[perm.size() - 1] = perm.size() - 2;
      return fmatmul(prev_adj, ftranspose(b, perm.data()));
    } else if (b == dx) {
      std::vector<int> perm(a->operation->dimensions);
      for (int i = 0; i < perm.size() - 2; i++)
        perm[i] = i;
      perm[perm.size() - 2] = perm.size() - 1;
      perm[perm.size() - 1] = perm.size() - 2;
      return fmatmul(ftranspose(a, perm.data()), prev_adj);
    } else {
      return nullptr;
    }
  }
  case FPOW: {
    FGraphNode *a = y->predecessors[0];
    FGraphNode *b = y->predecessors[1];
    if (a == dx) {
      // x^b / dx = b*x^(b-1)
      return fmul(prev_adj, fmul(b, fpow(a, fsub(b, 1))));
    } else if (b == dx) {
      // a^x / dx = a^x * ln(a)
      return fmul(prev_adj, fmul(fpow(a, b), flog(a)));
    } else
      return nullptr;
  }
  case FNEG: {
    FGraphNode *a = y->predecessors[0];
    if (a == dx) {
      return fneg(prev_adj);
    } else
      return nullptr;
  }
  case FLOG: {
    FGraphNode *a = y->predecessors[0];
    if (a == dx)
      return fdiv(prev_adj, a);
    else
      return nullptr;
  }
  case FLOG2: {
    FGraphNode *a = y->predecessors[0];
    if (a == dx)
      return fdiv(prev_adj, fmul(a, log(2.0)));
    else
      return nullptr;
  }
  case FLOG10: {
    FGraphNode *a = y->predecessors[0];
    if (a == dx)
      return fdiv(prev_adj, fmul(a, log(2.0)));
    else
      return nullptr;
  }
  case FLATTEN:
  case FCONVERSION:
  case FRESHAPE:
  case FMIN:
  case FMAX:
  case FREDUCE_SUM:
  case FREDUCE_MUL:
  case FSLICE:
  case FABS:
  case FREPEAT:
  case FTRANSPOSE:
  default:
    return nullptr;
  }
}

FGraphNode *fCalculateGradient(const FGraphNode *y, const FGraphNode *dx) {
  using namespace std;
  // to store gradients per node
  unordered_map<const FGraphNode *, FGraphNode *> adjoints;
  // fixpoint iteration
  list<const FGraphNode *> working;
  unordered_set<const FGraphNode *> in_working;
  working.push_back(y);
  in_working.insert(y);

  // initialize
  adjoints[y] = constant_tensor(1., y->operation->data_type,
                                y->operation->shape, y->operation->dimensions);
  FGraphNode *sol = nullptr;
  while (!working.empty()) {
    const FGraphNode *curr = working.front();
    working.pop_front();
    in_working.erase(curr);
    if (curr == dx) {
      sol = adjoints[curr];
    }
    if (curr->operation->op_type == FSTORE ||
        curr->operation->op_type == FCONST)
      continue;
    FGraphNode *adj = adjoints[curr];
    for (int i = 0; i < curr->num_predecessor; i++) {
      FGraphNode *parent = curr->predecessors[i];
      FGraphNode *local_grad = local_gradient(curr, parent, adj);
      if (adjoints.contains(parent)) {
        adjoints[parent] =
            fadd(adjoints[parent], unbroadcast(local_grad, parent));
      } else {
        adjoints.insert({parent, unbroadcast(local_grad, parent)});
      }
      if (!in_working.contains(parent)) {
        working.push_back(parent);
        in_working.insert(parent);
      }
    }
  }
  if (!sol)
    flogging(F_WARNING, "Operation graph did not contain the derivative!");
  return sol;
}
#endif
