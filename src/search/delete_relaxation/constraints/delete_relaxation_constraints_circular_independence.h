#ifndef DELETE_REALAXTION_CONSTRAINTS_CIRCULAR_INDEPENDENCE_H
#define DELETE_REALAXTION_CONSTRAINTS_CIRCULAR_INDEPENDENCE_H

#include "delete_relaxation_constraints_generator.h"
#include "delete_relaxation_constraints_time.h"
#include "../objectives/delete_relaxation_objective.h"
#include "../delete_relaxation_variables.h"
#include "../../lp_solver.h"
#include "../../task_proxy.h"

#include <set>
#include <sstream>
#include <string>
#include <map>

using std::map;
namespace DeleteRelaxation {

//bool threat(const OperatorProxy& threatee, const OperatorProy& threating) {
//
//}
bool preorder(const OperatorProxy& op1, const OperatorProxy& op2) {
	for (FactProxy prefact1 : op1.get_preconditions()) {
		for (FactProxy prefact2 : op2.get_preconditions()) {
			if (prefact1 == prefact2) {
				bool is_precondition_in_effect = false;
				for (EffectProxy effect1 : op1.get_effects()) {
					const FactProxy& efact1 = effect1.get_fact();
					if (prefact1.get_variable() == efact1.get_variable()) { // Same variable
						is_precondition_in_effect = true; // Op1 changes precondition prefact1
					}
				}

				if (!is_precondition_in_effect) {
					// Op1 does not change the truthness of prefact1

					for (EffectProxy effect2 : op2.get_effects()) {
						const FactProxy& efact2 = effect2.get_fact();
						if (prefact2.get_variable() == efact2.get_variable()
								&& prefact2.get_value() != efact2.get_value()) {
							// Op2 does change the truthness of prefact1 (&& values != is overkill)
							// op1 should precede op2 (if it is used)
							return true;
						}
					}
				}
			}
		}
	}
	return false;
}

class ConstraintsCircularIndependence: public ConstraintsTime {
	bool use_threat;
public:
	ConstraintsCircularIndependence(const Options&);

	virtual void update_constraints(double infinity, const State& initial_state,
			const TaskProxy& task_proxy,
			DeleteRelaxationVariables& lp_variables,
			const DeleteRelaxationModel& model) {
		ConstraintsTime::update_constraints(infinity, initial_state, task_proxy,
				lp_variables, model);
	}

	virtual void build_variables(double infinity, const State& initial_state,
			const TaskProxy& task_proxy,
			DeleteRelaxationVariables& lp_variables,
			const DeleteRelaxationObjective& model) {
		ConstraintsTime::build_variables(infinity, initial_state, task_proxy,
				lp_variables, model); // Create Time variables
	}

	virtual void build_constraints(double, const State&,
			const TaskProxy& task_proxy,
			DeleteRelaxationVariables& lp_variables,
			const DeleteRelaxationModel& model) {
//		UNUSED(infinity);
//		UNUSED(initial_state);

		const OperatorsProxy& ops = task_proxy.get_operators();
//
		// This is in original del rel
		// Forall(op Forall(p ∈ pre(op) [Time(p) <= Time(op)]))
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

		if (!model.use_inverse_actions()) {
			throw std::invalid_argument(
					"Model will fail without relevant and inverse actions and so is required to use inverse and relevant actions for constraint deleterelax_action_order");
		}
		/**
		 * THIS IS THE NEW CONSTRAINT
		 */
		// Forall(op, Forall(eff ∈ eff(op) [s.t not(Exists a' ∈ inv(op) with eff ∈ precondition(a')] [Time(op) + 1 <= Time(eff) + |OP|I(eff)]))
		for (size_t op_id = 0; op_id < ops.size(); ++op_id) {
			const OperatorProxy &op = ops[op_id];
			for (EffectProxy effect_proxy : op.get_effects()) {
				FactProxy effect = effect_proxy.get_fact();
				/*
				 * This prevents cycles within a causal graph appearing to prevent solvability
				 *
				 * variable.domain_size() > 2 is because if the variable is a true/false variable then
				 *   are expecting the Initial variable (I(p)) to resolve the cycle for us.
				 *   Because it must be the case that p is true or false in initial state.
				 */
				if ((lp_variables.inverse_actions_pre[op_id][effect].size() > 0
						&& effect.get_variable().get_domain_size() <= 2)) {
					int effect_id = effect.get_variable().get_id();
					int effect_value = effect.get_value();

					LPConstraint constraint(0, 0);
					constraint.insert(
							lp_variables.time_facts[effect_id][effect_value].index,
							1.0);
					constraint.insert(
							lp_variables.initialFacts[effect_id][effect_value].index,
							ops.size());
					constraint.insert(lp_variables.time_ops[op_id].index, -1.0);
					constraint.insert(lp_variables.LPONE.index, -1);
					lp_variables.constraints.push_back(constraint);

//					cout
//							<< lp_variables.variable_proxies[lp_variables.time_ops[op_id].index].get_details()
//							<< endl << " <<<<<<< " << endl
//							<< lp_variables.variable_proxies[lp_variables.time_facts[effect_id][effect_value].index].get_details()
//							<< endl;
				}
			}
		}

		// THREAT CONSTRAINT
		if (use_threat) {
//		OperatorsProxy ops = task_proxy.get_operators();
			// Forall(op1 ∈ Ops, Forall(op2 ∈ Ops, if(Exists(p ∈ pre(op1) \setintersects pre(op2) AND p is Unchanged ∈ eff(op1) AND p is Changed ∈ eff(op2) THEN T(op1) + epsilon <= T(op2)))))
			// Ideally this should be a non equality constraint, Time(op1) < Time(op2) instead of using epsilon.
			for (size_t op_id1 = 0; op_id1 < ops.size(); ++op_id1) {
				const OperatorProxy &op1 = ops[op_id1];

				for (size_t op_id2 = 0; op_id2 < ops.size(); ++op_id2) {
					const OperatorProxy &op2 = ops[op_id2];

					if (preorder(op1, op2)) {
						LPConstraint constraint(0, 0);
						constraint.insert(lp_variables.time_ops[op_id1].index,
								-1.0);
						constraint.insert(lp_variables.LPEPSILON.index, -1.0);
						constraint.insert(lp_variables.time_ops[op_id2].index,
								1.0);
						lp_variables.constraints.push_back(constraint);
					}
				}
			}
		}
	}
};
}
#endif
