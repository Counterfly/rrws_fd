#ifndef DELETE_RELAXATION_MIN_PLAN_COST_H
#define DELETE_RELAXATION_MIN_PLAN_COST_H

#include "delete_relaxation_objective.h"
#include "../../task_proxy.h"
#include <stdio.h>
#include <string>

namespace DeleteRelaxation {

class DeleteRelaxationMinPlanCost: public DeleteRelaxationObjective {

public:
	virtual double h_value(double objective_value, const Plan plan) const {
		UNUSED(plan);
		return objective_value;
	}

	virtual double get_used_op_coefficient(const OperatorProxy& op) const {
		if (op.get_name().find("bend") != std::string::npos)
			return 0.001;

		return op.get_cost();
	}
};

}

#endif
