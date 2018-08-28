#include "fetch_type_lazy.h"
#include "search_statistics.h"
#include "option_parser.h"
#include "plugin.h"

#define UNUSED(expr) do { (void)(expr); } while (0)

EvaluationContext FetchTypeLazy::evaluate_state(
		EvaluationContext& current_context, const GlobalOperator& op,
		const OperatorCost& cost_type) {
	GlobalState state = g_state_registry->get_successor_state(
			current_context.get_state(), op);
	int new_g_cost = current_context.get_g_value()
			+ get_adjusted_action_cost(op, cost_type);

	SearchStatistics& s = current_context.get_statistics();
	s.inc_generated();
	return EvaluationContext(state, new_g_cost, op.is_marked(), &s);
}

EvaluationContext FetchTypeLazy::expand_state(EvaluationContext& to_expand,
		const GlobalOperator& op, const OperatorCost& cost_type) {
	UNUSED(op);
	UNUSED(cost_type);
	return to_expand;
}

std::string FetchTypeLazy::to_string() {
	return "Lazy";
}

static FetchType* _parse(OptionParser &parser) {
    parser.document_synopsis("lazy heuristic evaluation.", "");

    Options opts = parser.parse();
    if (parser.dry_run())
        return 0;
    else
        return new FetchTypeLazy();
}

static Plugin<FetchType> _plugin("lazy", _parse);
