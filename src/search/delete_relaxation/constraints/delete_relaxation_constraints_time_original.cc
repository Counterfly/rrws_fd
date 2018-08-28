#include "delete_relaxation_constraints_time_original.h"
#include "delete_relaxation_constraints_time.h"

#include "../../option_parser.h"
#include "../../plugin.h"

namespace DeleteRelaxation {
ConstraintsTimeOriginal::ConstraintsTimeOriginal() {
}

static ConstraintsGenerator* _parse(OptionParser &parser) {
//	Options opts = parser.parse();
	if (parser.help_mode())
		return nullptr;
	if (parser.dry_run())
		return nullptr;
	return new ConstraintsTimeOriginal();
}

static Plugin<ConstraintsGenerator> _plugin("deleterelax_time_og", _parse);

}
