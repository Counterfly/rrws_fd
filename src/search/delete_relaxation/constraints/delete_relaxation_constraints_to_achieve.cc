#include "delete_relaxation_constraints_to_achieve.h"
#include "../delete_relaxation_variables.h"

#include "../../option_parser.h"
#include "../../plugin.h"

namespace DeleteRelaxation {

static ConstraintsGenerator* _parse(OptionParser &parser) {
	parser.add_option<int>("used_goal_le",
			"0/1. Make 0 if don't want constraints_original to pollute the 'used fact' namespace (will also need to set the parameter in constraints_original to >=0.",
			"1");
	Options opts = parser.parse();
	if (parser.help_mode())
		return nullptr;
	if (parser.dry_run())
		return nullptr;
	return new ConstraintsToAchieve(opts);
}

static Plugin<ConstraintsGenerator> _plugin("deleterelax_to_achieve", _parse);

}
