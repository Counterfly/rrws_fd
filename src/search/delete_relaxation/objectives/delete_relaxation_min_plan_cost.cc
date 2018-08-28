#include "delete_relaxation_min_plan_cost.h"
#include "delete_relaxation_objective.h"

#include "../option_parser.h"
#include "../plugin.h"

namespace DeleteRelaxation {

static DeleteRelaxationObjective* _parse(OptionParser &parser) {
	if (parser.help_mode())
		return nullptr;
	if (parser.dry_run())
		return nullptr;
	return new DeleteRelaxationMinPlanCost();
}

static Plugin<DeleteRelaxationObjective> _plugin("deleterelax_min_plan_cost", _parse);

}
