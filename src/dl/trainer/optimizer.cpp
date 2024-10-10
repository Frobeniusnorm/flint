#include "../trainer.hpp"
#include "flint.h"
#include <cmath>
#include <vector>
FGraphNode *Adam::optimize(FGraphNode *weight, FGraphNode *gradient) {
	if (m == nullptr) {
		if (weight->operation.data_type == F_FLOAT32) {
			m = fconstant_f(0.0, weight->operation.shape,
							weight->operation.dimensions);
			v = fconstant_f(0.0, weight->operation.shape,
							weight->operation.dimensions);
		} else {
			m = fconstant_d(0.0, weight->operation.shape,
							weight->operation.dimensions);
			v = fconstant_d(0.0, weight->operation.shape,
							weight->operation.dimensions);
		}
		m->reference_counter++;
		v->reference_counter++;
	}
	m = fadd_g(fmul_cf(m, b1), fmul_cf(gradient, (1 - b1)));
	v = fadd_g(fmul_cf(v, b2), fmul_g(gradient, fmul_cf(gradient, (1 - b2))));
	FGraphNode *mh = fdiv_cf(m, (1 - std::pow(b1, t)));
	FGraphNode *vh = fdiv_cf(v, (1 - std::pow(b2, t)));
	t += 1;
	return fsub_g(weight, fdiv_g(fmul_cf(mh, learning_rate),
								 fadd_cf(fsqrt_g(vh), epsilon)));
}
TrainingMetrics Trainer::train_epoch() {
	TrainingMetrics metrics;
	std::vector<FGraphNode *> weights(model->weights.size());
	for (int i = 0; i < weights.size(); i++)
		weights[i] = model->weights[i]->node;
	int total_batches = 0;
	metrics.training_loss = 0.0;
	while (data->remaining_for_epoch()) {
		auto [in_nodes, out_nodes] = data->next_batch();
		fStartGradientContext();
		auto output = model->operator()(in_nodes);
		std::vector<FGraphNode *> errors(output.size());
		for (int i = 0; i < output.size(); i++) {
			errors[i] = loss->calculate_loss(output[i], out_nodes[i]);
			errors[i]->reference_counter++;
		}
		fStopGradientContext();
		std::vector<FGraphNode *> gradients(weights.size());
		fCalculateGradients(errors[0], weights.data(), weights.size(),
							gradients.data());
		for (int i = 1; i < output.size(); i++) {
			std::vector<FGraphNode *> local_gradients(weights.size());
			fCalculateGradients(errors[i], weights.data(), weights.size(),
								local_gradients.data());
			// add to gradients
			for (int j = 0; j < local_gradients.size(); j++)
				gradients[j] = fadd_g(gradients[j], local_gradients[j]);
		}
		if (output.size() > 1)
			for (int j = 0; j < gradients.size(); j++)
				gradients[j] =
					fdiv_ci(gradients[j], errors.size()); // averaging
		// create average loss for reporting
		double batch_loss = 0.0;
		for (int i = 0; i < output.size(); i++) {
			errors[i]->reference_counter--; // no longer needed
			while (errors[i]->operation.dimensions > 1) {
				errors[i] =
					freduce_sum(errors[i], errors[i]->operation.dimensions - 1);
			}
			errors[i] = fconvert(freduce_sum(errors[i], 0), F_FLOAT32);
			batch_loss +=
				((float *)fCalculateResult(errors[i])->result_data->data)[0];
		}
		// optimizing
		for (int j = 0; j < gradients.size(); j++) {
			optimizer->optimize(weights[j], gradients[j]);
		}
		metrics.training_loss += batch_loss;
		total_batches++;
	}
	metrics.training_loss /= total_batches;
	return metrics;
}
void Trainer::train(size_t epochs) {
	for (size_t i = 0; i < epochs; i++) {
		TrainingMetrics metrics = train_epoch();
		// run validation
		auto [in_nodes, out_nodes] = data->validation_batch();
		auto output = model->operator()(in_nodes);
		double validation_error = 0.0;
		for (int i = 0; i < output.size(); i++) {
			FGraphNode *error = loss->calculate_loss(output[i], out_nodes[i]);
			while (error->operation.dimensions > 1) {
				error = freduce_sum(error, error->operation.dimensions - 1);
			}
			error = fconvert(freduce_sum(error, 0), F_FLOAT32);
			validation_error +=
				((float *)fCalculateResult(error)->result_data->data)[0];
		}
		metrics.validation_loss = validation_error;
		std::cout << "epoch " << i
				  << ": training loss: " << metrics.training_loss
				  << "validation loss: " << validation_error << std::endl;
	}
}
FGraphNode *CrossEntropyLoss::calculate_loss(FGraphNode *out, FGraphNode *exp) {
	const int n = out->operation.dimensions;
	auto pred = fmin_cd(fmax_cd(fexpand(fdiv_g(out, freduce_sum(out, n - 1)),
										n - 1, out->operation.shape[n - 1]),
								1e-7),
						1 - 1e-7);
	auto t1 = (fmul(exp, fneg(flog(pred))));
	while (t1->operation.dimensions > 1)
		t1 = freduce_sum(t1, 1);
	size_t total_size = 1;
	for (unsigned int i = 0; i < n - 1; i++)
		total_size *= out->operation.shape[i];
	return fdiv_cd(t1, (double)total_size);
}
