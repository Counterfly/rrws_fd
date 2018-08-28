#include "delete_relaxation_model.h"

#include "../option_parser.h"
#include "../plugin.h"
#include "../task_proxy.h"

#include <vector>
#include <set>
#include <map>
using std::map;
using std::vector;
using std::set;

namespace DeleteRelaxation {

void DeleteRelaxationModel::initialize(const TaskProxy& task_proxy) {
	UNUSED(task_proxy);
}
static DeleteRelaxationModel* _parse(OptionParser &parser) {

	parser.add_option<bool>("used_fact", "Used(fact)", "false");
	parser.add_option<bool>("used_op", "Used(operator)", "false");
	parser.add_option<bool>("initial_fact", "Init(fact)", "false");
	parser.add_option<bool>("first_achiever",
			"FirstAchiever(op, p). This is really the only one that should be changed.",
			"false");
	parser.add_option<bool>("time_fact", "Time(fact)", "false");
	parser.add_option<bool>("time_op", "Time(op)", "false");
	parser.add_option<bool>("prune_irrelevant", "Prune Irrelevant Facts",
			"false");
	parser.add_option<bool>("inverse_actions", "Use Inverse Actions in Model",
			"false");

	Options opts = parser.parse();
	if (parser.help_mode())
		return nullptr;
	if (parser.dry_run())
		return nullptr;
	return new DeleteRelaxationModel(opts);
}

static Plugin<DeleteRelaxationModel> _plugin("deleterelax_model", _parse);
}
