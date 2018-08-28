#include "fetch_type_eager.h"
#include "search_statistics.h"
#include "option_parser.h"
#include "plugin.h"

#define UNUSED(expr) do { (void)(expr); } while (0)

EvaluationContext FetchTypeEager::evaluate_state(
		EvaluationContext& current_context, const GlobalOperator& op,
		const OperatorCost& cost_type) {
	UNUSED(op);
	UNUSED(cost_type);
	return current_context;
}

EvaluationContext FetchTypeEager::expand_state(EvaluationContext& to_expand,
		const GlobalOperator& op, const OperatorCost& cost_type) {
	GlobalState state = g_state_registry->get_successor_state(
			to_expand.get_state(), op);
	int new_g_cost = to_expand.get_g_value()
			+ get_adjusted_action_cost(op, cost_type);
	SearchStatistics& s = to_expand.get_statistics();
	s.inc_generated();
	return EvaluationContext(state, new_g_cost, op.is_marked(), &s);
}

std::string FetchTypeEager::to_string() {
	return "Eager";
}

static FetchType* _parse(OptionParser &parser) {
    parser.document_synopsis("eager heuristic evaluation.", "");

    Options opts = parser.parse();
    if (parser.dry_run())
        return 0;
    else
        return new FetchTypeEager();
}

static Plugin<FetchType> _plugin("eager", _parse);
