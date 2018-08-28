#include "delete_relaxation_constraints_original.h"
#include "delete_relaxation_constraints_generator.h"
#include "../delete_relaxation_variables.h"

#include "../../option_parser.h"
#include "../../plugin.h"

#include <stdio.h>
using std::cout;
using std::endl;
#include <string>

namespace DeleteRelaxation {
const std::string ConstraintsOriginal::PLUGIN_KEY = "deleterelax_og";

ConstraintsOriginal::ConstraintsOriginal(const Options& opts) :
		ConstraintsGenerator(), used_goal_ge(opts.get<int>("used_goal_ge")) {
	assert(used_goal_ge >= 0 && used_goal_ge <= 1);
}

//void ConstraintsOriginal::add_state_to_constraints(const State &state,
//		double coefficient, DeleteRelaxationVariables& lp_variables) {
//	for (size_t var_id = 0; var_id < state.size(); ++var_id) {
//		FactProxy fact = state[var_id];
//		int value = fact.get_value();
//		int index = lp_variables.usedFacts[var_id][value].index;
//		LPConstraint constraint(1, 1);
//		constraint.insert(index, coefficient);
//		lp_variables.constraints.push_back(constraint);
//	}
//}

static ConstraintsGenerator* _parse(OptionParser &parser) {
	parser.add_option<int>("used_goal_ge",
			"The value to set Used(goal) >= value to. Original papers use 1 but should be considered if you are fine with polluting the 'used facts' namespace.",
			"1");
	Options opts = parser.parse();
	if (parser.help_mode())
		return nullptr;
	if (parser.dry_run())
		return nullptr;
	return new ConstraintsOriginal(opts);
}

static Plugin<ConstraintsGenerator> _plugin(ConstraintsOriginal::PLUGIN_KEY,
		_parse);
}
