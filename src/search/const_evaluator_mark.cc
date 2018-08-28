#include "const_evaluator_mark.h"
#include "evaluation_context.h"
#include "evaluation_result.h"

#include "option_parser.h"
#include "plugin.h"



#define UNUSED(expr) do { (void)(expr); } while (0)


ConstEvaluatorM::ConstEvaluatorM(const Options &opts) :
		value(opts.get<int>("value")) {
}

ConstEvaluatorM::ConstEvaluatorM(int v) :
		value(v) {
}

EvaluationResult ConstEvaluatorM::compute_result(
		EvaluationContext &eval_context) {
	UNUSED(eval_context);
	EvaluationResult result;
	result.set_h_value(value);
	return result;
}

static ScalarEvaluator *_parse(OptionParser &parser) {
	parser.document_synopsis("Constant evaluator", "Returns a constant value.");
	parser.add_option<int>("value", "the constant value", "0",
			Bounds("0", "infinity"));
	Options opts = parser.parse();
	if (parser.dry_run())
		return nullptr;
	else
		return new ConstEvaluatorM(opts);
}

static Plugin<ScalarEvaluator> _plugin("const", _parse);
