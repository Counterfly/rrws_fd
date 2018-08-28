#ifndef CONST_EVALUATOR_M_H
#define CONST_EVALUATOR_M_H

#include "scalar_evaluator.h"

class Options;

// TODO: When the searches don't need at least one heuristic anymore,
//       ConstEvaluator should inherit from ScalarEvaluator.
class ConstEvaluatorM: public ScalarEvaluator {
	int value;

public:
	explicit ConstEvaluatorM(const Options &opts);
	explicit ConstEvaluatorM(int v);
	virtual ~ConstEvaluatorM() override = default;
	virtual EvaluationResult compute_result(EvaluationContext &eval_context) override;
	virtual void get_involved_heuristics(std::set<Heuristic *> &) override {}
};

#endif
