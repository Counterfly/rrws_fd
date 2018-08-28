#ifndef DELETE_REALAXTION_CONSTRAINTS_TOACHIEVE_H
#define DELETE_REALAXTION_CONSTRAINTS_TOACHIEVE_H

#include "delete_relaxation_constraints_generator.h"
#include "../objectives/delete_relaxation_objective.h"
#include "../delete_relaxation_variables.h"

#include "../../lp_solver.h"
#include "../../task_proxy.h"
#include <set>
#include <sstream>
#include <string>
#include <map>

using std::vector;
using std::set;
namespace DeleteRelaxation {

class ConstraintsToAchieve: public ConstraintsGenerator {

	int used_goal_le;
public:
	ConstraintsToAchieve(const Options& opts) :
			ConstraintsGenerator(), used_goal_le(opts.get<int>("used_goal_le")) {
		assert(used_goal_le >= 0 && used_goal_le <= 1);
	}
	~ConstraintsToAchieve() {
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
			const DeleteRelaxationObjective&) {
		UNUSED(infinity);
		UNUSED(initial_state);

		// separate variables for goals not true in initial state that way they won't pollute the recursive structure of 'usedFacts' variable
		// Forall(g ∈ G, ToAchieve(g) ∈ {0,1} )
		for (FactProxy goal : task_proxy.get_goals()) {
			LPVariableWithIndex to_achieve(lp_variables.lp_variable_index++, 0,
					1, 0.0, lp_variables.is_used_fact_integer());
			lp_variables.facts_to_achieve[goal] = to_achieve;
			lp_variables.variables.push_back(to_achieve.get_lpvariable());
			lp_variables.variable_proxies.push_back(LPVariableProxy(goal));
		}

	}

	virtual void build_constraints(double infinity, const State& initial_state,
			const TaskProxy& task_proxy,
			DeleteRelaxationVariables& lp_variables,
			const DeleteRelaxationModel& model) {
		UNUSED(infinity);
		UNUSED(initial_state);
		UNUSED(model);

		vector<set<int>> *variable_achievers;
		set<int> *op_ids;

		for (FactProxy goal : task_proxy.get_goals()) {
			int var_id = goal.get_variable().get_id();
			int value = goal.get_value();

			// Forall(p ∈ {Goal-Init}  [ToAchieve(p) >= 1 - Init(p)])
			int index = lp_variables.facts_to_achieve[goal].index;
			LPConstraint constraint(1, 1);
			constraint.insert(index, 1.0);
			constraint.insert(lp_variables.initialFacts[var_id][value].index,
					1.0);
			lp_variables.constraints.push_back(constraint);

			// These goals cannot be 'Used', Used(goal) = 1
			// Used(goal) <= used_goal_le
			LPConstraint constraint2(0, 0);
			double coefficient;
			if (used_goal_le == 1) {
				// Used(goal) >= 0 + Init(g)
				coefficient = 1.0;
			} else {
				// -Used(goal) >= 0 - Init(g)	(Goals in Init can still have Used(g)=1)
				coefficient = -1.0;
			}
			constraint2.insert(lp_variables.usedFacts[var_id][value].index,
					coefficient);
			constraint2.insert(lp_variables.initialFacts[var_id][value].index,
					-coefficient);
			lp_variables.constraints.push_back(constraint2);
		}

		// This is part of the original del rel constraint below if using ToAchieve
		// This is equivalent to using (Used(p) - Init(p)) <= Sum(op s.t p ∈ eff(op) FirstAchiever(op, p))]
		//	but if we use ToAchieve, we need to force these goals to be FirstAchieved as Used(goal) is not forced to be True
		// Forall(goal ∈ {G-I} [ToAchieve(goal) - I(goal) <= Sum(op s.t goal ∈ eff(op) FirstAchiever(op, goal))])
		for (FactProxy goal : task_proxy.get_goals()) {
			int goal_id = goal.get_variable().get_id();
			int goal_value = goal.get_value();
			variable_achievers = &lp_variables.fact_achievers[goal_id];

//			if (initial_state[goal_id].get_value() != goal_value) {
			op_ids = &variable_achievers->at(goal_value);
			LPConstraint constraint(0, 0);

			constraint.insert(lp_variables.facts_to_achieve[goal].index, -1.0);
			constraint.insert(
					lp_variables.initialFacts[goal_id][goal_value].index, 1.0);

			FactProxyKey fact_key(goal);
			for (set<int>::iterator it = op_ids->begin(); it != op_ids->end();
					++it) {
				int op_id = *it;
				LPVariableWithIndex first_achiever_variable =
						lp_variables.first_achiever[op_id].find(fact_key)->second;
				constraint.insert(first_achiever_variable.index, 1.0);
			}
			lp_variables.constraints.push_back(constraint);
//			}
		}
	}
};
}
#endif
