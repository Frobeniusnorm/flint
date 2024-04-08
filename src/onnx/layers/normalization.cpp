#include "../layers.hpp"
#include "flint.h"
#include <limits>

void BatchNorm::forward() {
#ifdef FLINT_DEBUG
	if (incoming.size() != 5 || incoming[0]->output.size() != 1 ||
		incoming[1]->output.size() != 1 || incoming[2]->output.size() != 1)
		flogging(F_ERROR, "BatchNorm expects an image, a gamma, a beta, the "
						  "running mean and the running variance"
						  "parameter as inputs");
#endif
	FGraphNode *x = incoming[0]->output[0];
	FGraphNode *gamma = incoming[1]->output[0];
	FGraphNode *beta = incoming[2]->output[0];
	FGraphNode *mean_running = incoming[3]->output[0];
	FGraphNode *var_running = incoming[4]->output[0];
	Variable* node_mean = dynamic_cast<Variable*>(incoming[3]);
	Variable* node_var = dynamic_cast<Variable*>(incoming[4]);
#ifdef FLINT_DEBUG
	if (!node_mean || !node_var)
		flogging(F_ERROR, "4th and 5th parameter need to be input nodes for the running mean and variance.");
#endif
	// calculate mean
	if (training) {
		FGraphNode *mean =
			fdiv_ci(freduce_sum(x, 0), (unsigned int)x->operation.shape[0]);
		FGraphNode *var = fsub_g(x, mean);
		mean_running =
			fadd_g(fmul_cf(mean_running, alpha), fmul_cf(mean, 1 - alpha));
		var_running =
			fadd_g(fmul_cf(var_running, alpha), fmul_cf(var, 1 - alpha));
		node_mean->node = mean_running;
		node_var->node = var_running;
	}
	FGraphNode *y = fadd_g(
		fmul_g(gamma,
			   fdiv_g(fsub_g(x, mean_running),
					  fsqrt_g(fadd_cf(fpow(var_running, 2),
									  std::numeric_limits<float>::epsilon())))),
		beta);
  output[0] = y;
};