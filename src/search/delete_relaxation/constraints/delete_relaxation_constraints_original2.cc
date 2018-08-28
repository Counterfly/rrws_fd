#include "delete_relaxation_constraints_original2.h"
#include "delete_relaxation_constraints_generator.h"
#include "../delete_relaxation_variables.h"

#include "../../option_parser.h"
#include "../../plugin.h"

#include <stdio.h>
using std::cout;
using std::endl;
#include <string>

namespace DeleteRelaxation {

ConstraintsOriginal2::ConstraintsOriginal2(const Options& opts) :
		ConstraintsGenerator(), used_goal_ge(opts.get<int>("used_goal_ge")) {
	assert(used_goal_ge >= 0 && used_goal_ge <= 1);
}

static ConstraintsGenerator* _parse(OptionParser &parser) {
	parser.add_option<int>("used_goal_ge",
			"The value to set Used(goal) >= value to. Original papers use 1 but should be considered if you are fine with polluting the 'used facts' namespace.",
			"1");
	Options opts = parser.parse();
	if (parser.help_mode())
		return nullptr;
	if (parser.dry_run())
		return nullptr;
	return new ConstraintsOriginal2(opts);
}

static Plugin<ConstraintsGenerator> _plugin("deleterelax_og2", _parse);
}
