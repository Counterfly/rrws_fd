#include "delete_relaxation_constraints_circular_independence.h"
#include "../delete_relaxation_variables.h"

#include "../../option_parser.h"
#include "../../plugin.h"

namespace DeleteRelaxation {

ConstraintsCircularIndependence::ConstraintsCircularIndependence(
		const Options& opts) :
		use_threat(opts.get<bool>("threat")) {
}
static ConstraintsGenerator* _parse(OptionParser &parser) {
	parser.add_option<bool>("threat", "Use threat constraints.", "true");
	Options opts = parser.parse();
	if (parser.help_mode())
		return nullptr;
	if (parser.dry_run())
		return nullptr;
	return new ConstraintsCircularIndependence(opts);
}

static Plugin<ConstraintsGenerator> _plugin("deleterelax_action_order", _parse);
}
