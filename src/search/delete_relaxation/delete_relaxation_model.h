#ifndef DELETE_RELAXATION_MODEL_H
#define DELETE_RELAXATION_MODEL_H

#define UNUSED(expr) do { (void)(expr); } while (0)

#include "../option_parser.h"
#include "../task_proxy.h"
#include <stdio.h>
#include <vector>
#include <set>
#include <map>
using std::map;
using std::vector;
using std::set;
using std::cout;
using std::endl;

namespace DeleteRelaxation {

class DeleteRelaxationModel {

	/*
	 * Each boolean represents whether or not the associated variable is integer (if true) or linear (if false)
	 */
	bool used_fact, used_op, initial_fact, first_achiever, time_fact, time_op,
			prune_irrelevant, _use_inverse_actions;
public:
	DeleteRelaxationModel(const Options& opts) :
			used_fact(opts.get<bool>("used_fact")), used_op(
					opts.get<bool>("used_op")), initial_fact(
					opts.get<bool>("initial_fact")), first_achiever(
					opts.get<bool>("first_achiever")), time_fact(
					opts.get<bool>("time_fact")), time_op(
					opts.get<bool>("time_op")), prune_irrelevant(
					opts.get<bool>("prune_irrelevant")), _use_inverse_actions(
					opts.get<bool>("inverse_actions")) {
	}
	~DeleteRelaxationModel() {
	}

	virtual void initialize(const TaskProxy&);
	/*
	 * Returns true as long as all variables are not all linear (at least one variable is integer).
	 */
	bool is_mip() const {
		return used_fact || used_op || first_achiever || time_fact || time_op;
	}

	/*
	 * Returns true if all variables are non-integer
	 */
	bool is_linear() const {
		return !is_mip();
	}

	bool is_initial_fact_integer() const {
		return initial_fact;
	}

	bool is_used_fact_integer() const {
		return used_fact;
	}

	bool is_used_op_integer() const {
		return used_op;
	}

	bool is_first_achiever_integer() const {
		return first_achiever;
	}

	bool is_time_fact_integer() const {
		return time_fact;
	}

	bool is_time_op_integer() const {
		return time_op;
	}

	bool is_prune_irrelevant() const {
		return prune_irrelevant;
	}

	bool use_inverse_actions() const {
		return _use_inverse_actions;
	}
};
} // END DelRel namespace

#endif
