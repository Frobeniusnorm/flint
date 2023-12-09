#include "sliding_windows.hpp"

using namespace std;

template <typename T>
void SlidingWindowImpl::unary_expression(T *__restrict__ result,
										 const T *__restrict__ data,
										 size_t from, size_t size,
										 const FGraphNode *curr) {
	const FOperation pred = curr->predecessors[0]->operation;
	const FSlidingWindow *slidewin =
		(FSlidingWindow *)curr->operation.additional_data;
	size_t acc_size = curr->operation.shape[1];
	std::vector<size_t> acc_sizes_pred =
		calcAccSizes(pred.dimensions, pred.shape);
	std::vector<size_t> acc_sizes_win(pred.dimensions);
	std::vector<size_t> acc_sizes_rest(pred.dimensions);
	acc_sizes_win[acc_sizes_win.size() - 1] = 1;
	acc_sizes_rest[acc_sizes_win.size() - 1] = 1;
	for (int i = acc_sizes_pred.size() - 2; i >= 0; i--) {
		acc_size *= curr->operation.shape[i + 2];
		acc_sizes_rest[i] = acc_sizes_rest[i + 1] * slidewin->size[i + 1];
		// no of windows in that dimension
		size_t window_size = pred.shape[i + 1] - slidewin->size[i + 1] + 1;
		window_size = window_size % slidewin->step[i + 1] == 0
						  ? window_size / slidewin->step[i + 1]
						  : window_size / slidewin->step[i + 1] + 1;
		acc_sizes_win[i] = acc_sizes_win[i + 1] * window_size;
	}
	for (size_t i = from; i < from + size; i++) {
		// window number
		size_t wi = i / acc_size;
		size_t rest = i % acc_size;
		// calculate window base from wi
		size_t base = 0;
		// index per dimension inside window from rest
		size_t offset = 0;
		for (int d = 0; d < pred.dimensions; d++) {
			size_t local_wi = wi / acc_sizes_win[d];
			// top left corner of window in that dimension
			size_t loc_base = local_wi * slidewin->step[d];
			base += loc_base * acc_sizes_pred[d];
			// remove this dimension from wi
			wi %= acc_sizes_win[d];
			size_t local_ri = rest / acc_sizes_rest[d];
			offset += local_ri * acc_sizes_pred[d];
			rest %= acc_sizes_rest[d];
		}
		result[i] = data[base + offset];
	}
}
int SlidingWindowImpl::generate_ocl_lazy(const FGraphNode *node,
										 std::string name,
										 OCLLazyCodegenState &compiler_state) {
	const FOperation pred = node->predecessors[0]->operation;
	const FSlidingWindow *slidewin =
		(FSlidingWindow *)node->operation.additional_data;
	size_t acc_size = node->operation.shape[1];
	vector<size_t> acc_sizes_pred(pred.dimensions);
	vector<size_t> acc_sizes_win(pred.dimensions);
	vector<size_t> acc_sizes_rest(pred.dimensions);
	acc_sizes_pred[acc_sizes_pred.size() - 1] = 1;
	acc_sizes_win[acc_sizes_win.size() - 1] = 1;
	acc_sizes_rest[acc_sizes_win.size() - 1] = 1;
	for (int i = acc_sizes_pred.size() - 2; i >= 0; i--) {
		acc_size *= node->operation.shape[i + 2];
		acc_sizes_pred[i] = acc_sizes_pred[i + 1] * pred.shape[i + 1];
		acc_sizes_rest[i] = acc_sizes_rest[i + 1] * slidewin->size[i + 1];
		// no of windows in that dimension
		size_t window_size = pred.shape[i + 1] - slidewin->size[i + 1] + 1;
		window_size = window_size % slidewin->step[i + 1] == 0
						  ? window_size / slidewin->step[i + 1]
						  : window_size / slidewin->step[i + 1] + 1;
		acc_sizes_win[i] = acc_sizes_win[i + 1] * window_size;
	}
	const size_t num_elems = acc_size * node->operation.shape[0];
	const unsigned int old_idx = compiler_state.num_indices++;
	const std::string i = "old_index" + to_string(old_idx);
	Twine index_defs;
	index_defs += "long " + i +
				  " = index;\n"
				  "index = 0;\n{\n"
				  "long wi = (" +
				  i + "%" + to_string(num_elems) + ")/" + to_string(acc_size) +
				  ";\n"
				  "long rest = " +
				  i + "%" + to_string(acc_size) + ";\n";
	for (int d = 0; d < pred.dimensions; d++) {
		std::string local_wi = "wi/" + to_string(acc_sizes_win[d]);
		std::string loc_base = local_wi + "*" + to_string(acc_sizes_pred[d]) +
							   "*" + to_string(slidewin->step[d]);
		std::string local_ri = "rest/" + to_string(acc_sizes_rest[d]) + "*" +
							   to_string(acc_sizes_pred[d]);
		index_defs += "index += " + loc_base + " + " + local_ri +
					  ";\n"
					  "wi %= " +
					  to_string(acc_sizes_win[d]) +
					  ";\n"
					  "rest %= " +
					  to_string(acc_sizes_rest[d]) + ";\n";
	}
	index_defs += "}\n";
	compiler_state.index_defs = index_defs;
	compiler_state.code.prepend(
		"const " + typeString(node->operation.data_type) + " " + name + " = v" +
		to_string(compiler_state.variable_index + 1) +
		";\n"
		"index = old_index" +
		to_string(old_idx) + ";\n");
	return 0;
}
std::string
SlidingWindowImpl::generate_ocl_eager(FType res_type,
									  std::vector<FType> parameter_types) {}
void SlidingWindowImpl::execute_cpu(const FGraphNode *node,
									std::vector<CPUResultData> predecessor_data,
									void *__restrict__ result, size_t from,
									size_t size) {
	UNARY_EXECUTE_MONOTON_IMPL
}
template <typename T>
void UnslideWindowImpl::unary_expression(T *__restrict__ result,
										 const T *__restrict__ data,
										 size_t from, size_t size,
										 const FGraphNode *curr) {
	const FOperation pred = curr->predecessors[0]->operation;
	const unsigned int *steps = (unsigned int *)curr->operation.additional_data;
	const std::vector<size_t> acc_sizes =
		calcAccSizes(curr->operation.dimensions, curr->operation.shape);
	const std::vector<size_t> acc_sizes_pred =
		calcAccSizes(pred.dimensions, pred.shape);
	size_t no_windows[pred.dimensions - 1];
	for (int i = 0; i < pred.dimensions - 1; i++) {
		size_t window_size = curr->operation.shape[i] - pred.shape[i + 1] + 1;
		no_windows[i] = window_size % steps[i] == 0
							? window_size / steps[i]
							: window_size / steps[i] + 1;
	}
	const std::vector<size_t> acc_no_windows =
		calcAccSizes(pred.dimensions - 1, no_windows);
	for (size_t i = from; i < from + size; i++) {
		result[i] = 0;
		size_t first_w = 0, last_w = 0;
		// calculate first and last hit
		for (int d = 0; d < curr->operation.dimensions; d++) {
			const unsigned int id =
				(i / acc_sizes[d]) % curr->operation.shape[d];
			// first hit is where the window overlaps with the element of
			// window size - 1 before this element (since the window reaches
			// to this element)
			const size_t wdf = (size_t)std::ceil(
				(std::max(0l, (long)id - (long)pred.shape[d + 1] + 1) /
				 (double)steps[d]));
			const size_t wfl = id / steps[d];
			first_w += wdf * acc_no_windows[d];
			last_w += wfl * acc_no_windows[d];
		}
		size_t w = first_w;
		while (w <= last_w) {
			// tests if this window is a hit or not, if not calculates the
			// distance to the next window
			bool contained = true;
			size_t wi = 0;
			size_t wpp = 0;
			for (int d = curr->operation.dimensions - 1; d >= 0; d--) {
				const unsigned int wd = (w / acc_no_windows[d]) % no_windows[d];
				const unsigned int w_start = wd * steps[d];
				const unsigned int id =
					(i / acc_sizes[d]) % curr->operation.shape[d];
				if (id >= w_start && id < w_start + pred.shape[d + 1]) {
					wi += (id - w_start) * acc_sizes_pred[d + 1];
				} else {
					contained = false;
					// we cant break yet -> advance to next window in
					// dimension
					wpp += acc_no_windows[d];
				}
			}
			if (contained) {
				result[i] += data[wi + w * acc_sizes_pred[0]];
				wpp = 1;
			}
			w += wpp;
		}
	}
}
int UnslideWindowImpl::generate_ocl_lazy(const FGraphNode *node,
										 std::string name,
										 OCLLazyCodegenState &compiler_state) {

	FGraphNode *gnp1 = node->predecessors[0];
	const FOperation pred = gnp1->operation;
	const string par1 = compiler_state.findOrInsertParameter(gnp1);
	const unsigned int *steps = (unsigned int *)node->operation.additional_data;
	const vector<size_t> acc_sizes =
		calcAccSizes(node->operation.dimensions, node->operation.shape);
	const vector<size_t> acc_sizes_pred =
		calcAccSizes(pred.dimensions, pred.shape);
	size_t no_windows[pred.dimensions - 1];
	for (int i = 0; i < pred.dimensions - 1; i++) {
		size_t window_size = node->operation.shape[i] - pred.shape[i + 1] + 1;
		no_windows[i] = window_size % steps[i] == 0
							? window_size / steps[i]
							: window_size / steps[i] + 1;
	}
	const vector<size_t> acc_no_windows =
		calcAccSizes(pred.dimensions - 1, no_windows);
	Twine local_code = typeString(node->operation.data_type) + " " + name +
					   " = 0;\n"
					   "{\n"
					   "const long first_w = 0";
	for (int d = node->operation.dimensions - 1; d >= 0; d--) {
		local_code += " + max(0l, ((index / " + to_string(acc_sizes[d]) +
					  ") % " + to_string(node->operation.shape[d]) + ") - " +
					  to_string(pred.shape[d + 1]) + " + 1) / " +
					  to_string(steps[d]) + " * " +
					  to_string(acc_no_windows[d]);
	}
	local_code += ";\nconst long last_w = 0";
	for (int d = node->operation.dimensions - 1; d >= 0; d--) {
		local_code += " + ((index / " + to_string(acc_sizes[d]) + ") % " +
					  to_string(node->operation.shape[d]) + ") / " +
					  to_string(steps[d]) + " * " +
					  to_string(acc_no_windows[d]);
	}
	local_code += ";\nfor(long w=first_w;w<=last_w;){\n"
				  " bool contained = true;\n"
				  " long wi = 0;\n"
				  " long wpp = 0;\n";
	for (int d = node->operation.dimensions - 1; d >= 0; d--) {
		local_code += " {\n"
					  "  const long w_start=((w/" +
					  to_string(acc_no_windows[d]) + ")%" +
					  to_string(no_windows[d]) + ")*" + to_string(steps[d]) +
					  ";\n"
					  "  const long id=(index/" +
					  to_string(acc_sizes[d]) + ")%" +
					  to_string(node->operation.shape[d]) +
					  ";\n"
					  "  if(id>=w_start && id<w_start+" +
					  to_string(pred.shape[d + 1]) +
					  ")\n"
					  "   wi+=(id-w_start)*" +
					  to_string(acc_sizes_pred[d + 1]) +
					  ";\n"
					  "  else{\n"
					  "   contained = false;\n"
					  "   wpp += " +
					  to_string(acc_no_windows[d]) +
					  ";\n"
					  "  }\n"
					  " }\n";
	}
	local_code += " if(contained) {"
				  "  " +
				  name + "+=" + par1 + "[wi+w*" + to_string(acc_sizes_pred[0]) +
				  "];\n"
				  "  wpp = 1;\n}\n"
				  " w += wpp;\n"
				  "}\n"
				  "}";
	compiler_state.code.prepend(local_code);
	return OCL_LAZY_DONT_PUSH_PREDS;
}
std::string
UnslideWindowImpl::generate_ocl_eager(FType res_type,
									  std::vector<FType> parameter_types) {}
void UnslideWindowImpl::execute_cpu(const FGraphNode *node,
									std::vector<CPUResultData> predecessor_data,
									void *__restrict__ result, size_t from,
									size_t size) {
	UNARY_EXECUTE_MONOTON_IMPL
}
