#ifndef DELETE_RELAXATION_CONSTRAINTS_TIME_H
#define DELETE_RELAXATION_CONSTRAINTS_TIME_H

#include "delete_relaxation_constraints_generator.h"
#include "../objectives/delete_relaxation_objective.h"
#include "../delete_relaxation_variables.h"
#include "../../lp_solver.h"
#include "../../task_proxy.h"

#include <set>
#include <sstream>
#include <string>
#include <map>
#include <stdio.h>

namespace DeleteRelaxation {
class DeleteRelaxationObjective;
using std::endl;
using std::string;
using std::vector;
using std::map;
using std::set;
using std::cout;

class ConstraintsTime: public ConstraintsGenerator {

public:
	ConstraintsTime() {
	}
	~ConstraintsTime() {
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
		UNUSED(initial_state);

		VariablesProxy vars = task_proxy.get_variables();
		OperatorsProxy ops = task_proxy.get_operators();

		// Time each Proposition is achieved, Time(p)=0 means p started true in init state.
		for (VariableProxy var : vars) {
			int var_id = var.get_id();
			lp_variables.time_facts.push_back(vector<LPVariableWithIndex>());
			for (int value = 0; value < var.get_domain_size(); ++value) {
				FactProxy fact = var.get_fact(value);
				LPVariableWithIndex time(lp_variables.lp_variable_index++, 0,
						ops.size(), objective.get_time_fact_coefficient(fact),
						lp_variables.is_time_fact_integer());
				lp_variables.time_facts[var_id].push_back(time);
				lp_variables.variables.push_back(time.get_lpvariable());
				lp_variables.variable_proxies.push_back(
						LPVariableProxy(fact, string("Time")));
			}
		}

		// Time each Action is used in plan. Time(op)=0 means op is first action, Time(op)=|OP| means op is never used.
		for (size_t op_id = 0; op_id < ops.size(); ++op_id) {
			const OperatorProxy &op = ops[op_id];

			LPVariableWithIndex time(lp_variables.lp_variable_index++, 0,
					ops.size(), objective.get_time_op_coefficient(op),
					lp_variables.is_time_op_integer());
			lp_variables.time_ops.push_back(time);
			lp_variables.variables.push_back(time.get_lpvariable());
			lp_variables.variable_proxies.push_back(
					LPVariableProxy(ops[op_id], string("Time")));
		}

	}

	virtual void build_constraints(double infinity, const State& initial_state,
			const TaskProxy& task_proxy,
			DeleteRelaxationVariables& lp_variables,
			const DeleteRelaxationModel& model) = 0;
};
} // END DelRel namespace

#endif
