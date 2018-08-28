#ifndef DELETE_RELAXATION_CONSTRAINTS_TIME_OG_H
#define DELETE_RELAXATION_CONSTRAINTS_TIME_OG_H

#include "delete_relaxation_constraints_generator.h"
#include "../objectives/delete_relaxation_objective.h"
#include "delete_relaxation_constraints_time.h"
#include "../delete_relaxation_variables.h"
#include "../../lp_solver.h"
#include "../../task_proxy.h"

#include <set>
#include <sstream>
#include <string>
#include <map>

namespace DeleteRelaxation {
class DeleteRelaxationObjective;
using std::endl;
using std::string;
using std::vector;
using std::map;
using std::set;

class ConstraintsTimeOriginal: public ConstraintsTime {

public:
	ConstraintsTimeOriginal();
	~ConstraintsTimeOriginal() {
	}

	virtual void update_constraints(double infinity, const State& initial_state,
			const TaskProxy& task_proxy,
			DeleteRelaxationVariables& lp_variables,
			const DeleteRelaxationModel& model) {
		ConstraintsTime::update_constraints(infinity, initial_state, task_proxy,
				lp_variables, model);
	}

	virtual void build_constraints(double infinity, const State& initial_state,
			const TaskProxy& task_proxy,
			DeleteRelaxationVariables& lp_variables,
			const DeleteRelaxationModel& model) {
		UNUSED(infinity);
		UNUSED(initial_state);
		UNUSED(model);

		OperatorsProxy ops = task_proxy.get_operators();

		// This is in original del rel
		// Forall(op Forall(p ∈ pre(op) [Time(p) <= Time(op)])) == Time(op) >= Time(p)
		for (size_t op_id = 0; op_id < ops.size(); ++op_id) {
			const OperatorProxy &op = ops[op_id];
			for (const FactProxy& p : op.get_preconditions()) {
				LPConstraint constraint(0, 0);
				constraint.insert(
						lp_variables.time_facts[p.get_variable().get_id()][p.get_value()].index,
						-1.0);
				constraint.insert(lp_variables.time_ops[op_id].index, 1.0);
				lp_variables.constraints.push_back(constraint);
			}
		}

		// This is in original del rel
		//	To + 1 ≤ Ta + M(1 − Fo,a) f.a. o ∈ O, a ∈ add(o)
		//	Forall(op Forall(p ∈ eff(op) [Time(op) + 1 <= Time(p) + |OP|(1 - FirstAchiever(op, p))]))
		double M = ops.size() + 1;
		int var_id, value;
		for (size_t op_id = 0; op_id < ops.size(); ++op_id) {
			const OperatorProxy &op = ops[op_id];

			for (EffectProxy effect_proxy : op.get_effects()) {
				LPConstraint constraint(0, 0);
				FactProxy fact = effect_proxy.get_fact();
				var_id = fact.get_variable().get_id();
				value = fact.get_value();

				constraint.insert(lp_variables.time_facts[var_id][value].index,
						1.0);
				constraint.insert(lp_variables.LPONE.index, M);
				constraint.insert(
						lp_variables.first_achiever[op_id][fact].index, -M);

				constraint.insert(lp_variables.time_ops[op_id].index, -1.0);
				constraint.insert(lp_variables.LPONE.index, -1.0);
				lp_variables.constraints.push_back(constraint);
			}
		}
	}
};
} // END DelRel namespace

#endif
