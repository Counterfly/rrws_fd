#ifndef FETCH_TYPE_H
#define FETCH_TYPE_H

#include "evaluation_context.h"
#include "global_operator.h"
#include "operator_cost.h"

class FetchType {

public:
    virtual EvaluationContext evaluate_state(EvaluationContext& current_context, const GlobalOperator& op, const OperatorCost& cost_type) = 0;
    virtual EvaluationContext expand_state(EvaluationContext& to_expand, const GlobalOperator& op, const OperatorCost& cost_type) = 0;
    virtual std::string to_string() = 0;
    virtual void dummy_function() {};

    FetchType() {};
    virtual ~FetchType() {};
};

#endif
