#ifndef DELETE_RELAXATION_CONSTRAINTS_OG2_H
#define DELETE_RELAXATION_CONSTRAINTS_OG2_H

#include "delete_relaxation_constraints_generator.h"
#include "../objectives/delete_relaxation_objective.h"
#include "../delete_relaxation_variables.h"
#include "../../lp_solver.h"
#include "../../task_proxy.h"

#include <set>
#include <sstream>
#include <string>
#include <map>

using std::endl;
using std::string;
using std::vector;
using std::map;
using std::set;
namespace DeleteRelaxation {
class DeleteRelaxationObjective;

// EXCLUDES the First Achiever Variables
class ConstraintsOriginal2: public ConstraintsGenerator {

	int used_goal_ge;

public:
	static const std::string PLUGIN_KEY;
	ConstraintsOriginal2(const Options&);
	~ConstraintsOriginal2() {
	}

	virtual void update_constraints(double infinity, const State& initial_state,
			const TaskProxy& task_proxy,
			DeleteRelaxationVariables& lp_variables,
			const DeleteRelaxationModel& model) {
		ConstraintsGenerator::update_constraints(infinity, initial_state, task_proxy,
				lp_variables, model);
	}

	virtual void build_variables(double infinity, const State& initial_state,
			const TaskProxy& task_proxy,
			DeleteRelaxationVariables& lp_variables,
			const DeleteRelaxationObjective& objective) {
		UNUSED(infinity);

		VariablesProxy vars = task_proxy.get_variables();

		// variable is a proposition->value pair
		// Used Facts
		int var_id;
		for (VariableProxy var : vars) {
			var_id = var.get_id();
			lp_variables.usedFacts.push_back(vector<LPVariableWithIndex>());
			lp_variables.fact_achievers.push_back(vector<set<int>>());

			for (int value = 0; value < var.get_domain_size(); ++value) {
				double coefficient = objective.get_used_fact_coefficient(
						var.get_fact(value));
//				lp_variables.add_used_fact(var.get_fact(value), coefficient);
				LPVariableWithIndex usedFact(lp_variables.lp_variable_index++,
						0, 1, coefficient, lp_variables.is_used_fact_integer());
				lp_variables.usedFacts[var_id].push_back(usedFact);
				lp_variables.variables.push_back(usedFact.get_lpvariable());
				lp_variables.variable_proxies.push_back(
						LPVariableProxy(var.get_fact(value)));

				lp_variables.fact_achievers[var_id].push_back(set<int>());
			}
		}

		// Initial Facts
		for (VariableProxy var : vars) {
			var_id = var.get_id();
			lp_variables.initialFacts.push_back(vector<LPVariableWithIndex>());

			for (int value = 0; value < var.get_domain_size(); ++value) {
				double coefficient = 0.0; //objective.get_init_fact_coefficient(var.get_fact(value));
				//				lp_variables.add_initial_fact(var.get_fact(value), coefficient);

				// If fact is present in initial state then it has value 1, otherwise 0
				double init_value;
				if (initial_state[var_id].get_value() == value)
					init_value = 1.0;
				else {
					init_value = 0.0;
				}
				LPVariableWithIndex initFact(lp_variables.lp_variable_index++,
						init_value, init_value, coefficient,
						lp_variables.is_initial_fact_integer());
				lp_variables.initialFacts[var_id].push_back(initFact);
				lp_variables.variables.push_back(initFact.get_lpvariable());
				lp_variables.variable_proxies.push_back(
						LPVariableProxy(var.get_fact(value)));
			}
		}

		// Operators that achieve certain Facts
		OperatorsProxy ops = task_proxy.get_operators();
		lp_variables.usedOps.reserve(ops.size());
		for (size_t op_id = 0; op_id < ops.size(); ++op_id) {
			const OperatorProxy &op = ops[op_id];

			double coefficient = objective.get_used_op_coefficient(op);
			LPVariableWithIndex usedOp(lp_variables.lp_variable_index++, 0, 1,
					coefficient, lp_variables.is_used_op_integer());
			lp_variables.usedOps.push_back(usedOp);
			lp_variables.variables.push_back(usedOp.get_lpvariable());
			lp_variables.variable_proxies.push_back(LPVariableProxy(op));

			for (EffectProxy effect_proxy : op.get_effects()) {
				FactProxy effect = effect_proxy.get_fact();
				int var = effect.get_variable().get_id();
				int post = effect.get_value();
				lp_variables.fact_achievers[var][post].insert(op_id);
			}
		}
	}

	virtual void build_constraints(double infinity, const State& initial_state,
			const TaskProxy& task_proxy,
			DeleteRelaxationVariables& lp_variables,
			const DeleteRelaxationModel& model) {
		UNUSED(infinity);
		UNUSED(initial_state);
		UNUSED(model);

		OperatorsProxy ops = task_proxy.get_operators();

		// [Now it is Used(p)>=used_goal_ge]
		for (FactProxy goal : task_proxy.get_goals()) {
			int var_id = goal.get_variable().get_id();
			int value = goal.get_value();

			//Forall(p in goal state, Used(p)=1)
			int index = lp_variables.usedFacts[var_id][value].index;
			LPConstraint constraint(used_goal_ge, used_goal_ge);
			constraint.insert(index, 1.0);
			lp_variables.constraints.push_back(constraint);
		}

		set<int>* inv_actions;
		int inv_op_id;
		// This is in original del rel
		// Forall(op Forall(p ∈ pre(op) Used(p) - Sum(Used(a) Forall(a ∈ Inv(op,p))) >= Used(op)))
		for (size_t op_id = 0; op_id < ops.size(); ++op_id) {
			const OperatorProxy &op = ops[op_id];
			for (FactProxy p : op.get_preconditions()) {
				LPConstraint constraint(0, 0);
				constraint.insert(lp_variables.usedOps[op_id].index, -1.0);
				constraint.insert(
						lp_variables.usedFacts[p.get_variable().get_id()][p.get_value()].index,
						1.0);

				if (model.use_inverse_actions()) {
					inv_actions = &lp_variables.inverse_actions_eff[op_id][p];
					for (set<int>::iterator it = inv_actions->begin();
							it != inv_actions->end(); ++it) {
						inv_op_id = *it;
						constraint.insert(lp_variables.usedOps[inv_op_id].index,
								-1);
					}
				}
				lp_variables.constraints.push_back(constraint);
			}
		}

		set<int> *op_ids;
		/*
		 * Time Sensitive constraints
		 */

		int var_id;
		// TODO: determine which constraint is better among next 3
		// This is in original del rel...CHANGED to remove FirstAchieved
		// Forall(op Forall(p ∈ eff(op) Used(op) >= FirstAchiever(op, p))
		// NEW: Forall(p ∈ P [Sum(Used(a) | p ∈ eff(a)) >= (Used(p) - Init(p))])
		for (VariableProxy var : task_proxy.get_variables()) {
			var_id = var.get_id();

			for (int value = 0; value < var.get_domain_size(); ++value) {
//				const FactProxy& fact = var.get_fact(value);

				LPConstraint constraint(0, 0);
				constraint.insert(lp_variables.usedFacts[var_id][value].index,
						-1);
				constraint.insert(
						lp_variables.initialFacts[var_id][value].index, 1);

				op_ids = &lp_variables.fact_achievers[var_id][value];
				for (set<int>::iterator it = op_ids->begin();
						it != op_ids->end(); ++it) {
					int op_id = *it;
					constraint.insert(lp_variables.usedOps[op_id].index, 1.0);
				}
				lp_variables.constraints.push_back(constraint);
			}
		}

		/*****
		 * NOTE: The constraints below act as the recursive initiators that make ops and facts 'used'
		 *****/
		// This is in original del rel
		// Forall(p [(Used(p) - Init(p)) <= Sum(op s.t p ∈ eff(op) FirstAchiever(op, p))]) - Similar to using Used(op) i think
		// NEW: Forall(p ∈ P, [Used(p) - Init(p) <= Sum(Used(op) s.t p ∈ eff(op))])
		for (VariableProxy var : task_proxy.get_variables()) {
			var_id = var.get_id();

			for (int var_value = 0; var_value < var.get_domain_size();
					++var_value) {

				LPConstraint constraint(0, 0);
				constraint.insert(
						lp_variables.initialFacts[var_id][var_value].index,
						1.0);
				constraint.insert(
						lp_variables.usedFacts[var_id][var_value].index, -1.0);

				op_ids = &lp_variables.fact_achievers[var_id][var_value];
				for (set<int>::iterator it = op_ids->begin();
						it != op_ids->end(); ++it) {
					int op_id = *it;
					constraint.insert(lp_variables.usedOps[op_id].index, 1.0);
				}
				lp_variables.constraints.push_back(constraint);
			}
		}
	}
}
;
} // END DelRel namespace

#endif
