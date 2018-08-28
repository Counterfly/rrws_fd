#ifndef FETCH_TYPE_LAZY_H
#define FETCH_TYPE_LAZY_H

#include "fetch_type.h"
#include "evaluation_context.h"
#include "global_operator.h"
#include "operator_cost.h"

class FetchTypeLazy : public FetchType {

public:
	EvaluationContext evaluate_state(EvaluationContext& current_context, const GlobalOperator& op, const OperatorCost& cost_type);
    EvaluationContext expand_state(EvaluationContext& to_expand, const GlobalOperator& op, const OperatorCost& cost_type);
    std::string to_string();

    FetchTypeLazy() {};
    ~FetchTypeLazy() {};
};

#endif