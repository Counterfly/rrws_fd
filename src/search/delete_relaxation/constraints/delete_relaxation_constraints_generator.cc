#include "delete_relaxation_constraints_generator.h"

namespace DeleteRelaxation {
void ConstraintsGenerator::update_constraints(double /*infinity*/,
		const State& initial_state, const TaskProxy& task_proxy,
		DeleteRelaxationVariables& lp_variables,
		const DeleteRelaxationModel& model) {
	// Reset value for Initial Variables
	int index, var_id, value;
	for (const FactProxy fact : task_proxy.get_variables().get_facts()) {
		var_id = fact.get_variable().get_id();
		value = fact.get_value();
		index = lp_variables.initialFacts[var_id][value].index;
		int initial_value = 0;
		if (initial_state[var_id].get_value() == value) {
			initial_value = 1;
		}
		LPVariable variable(initial_value, initial_value, 0,
				model.is_initial_fact_integer());
		lp_variables.variables[index] = variable;
	}
}
}
