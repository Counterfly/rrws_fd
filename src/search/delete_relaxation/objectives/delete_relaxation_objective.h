#ifndef DELETE_RELAXATION_OBJ_H
#define DELETE_RELAXATION_OBJ_H

#define UNUSED(expr) do { (void)(expr); } while (0)

#include "../../task_proxy.h"
#include "../../search_engine.h"
typedef SearchEngine::Plan Plan;

namespace DeleteRelaxation {

class DeleteRelaxationObjective {

public:
//	virtual DeleteRelaxationObjective get_objective();
	virtual double h_value(double objective_value, const Plan plan) const = 0;

	virtual double get_used_fact_coefficient(const FactProxy& f) const {
		UNUSED(f);
		return 0.0;
	}
	virtual double get_used_op_coefficient(const OperatorProxy& op) const {
		UNUSED(op);
		return 0.0;
	}

	virtual double get_first_achiever_coefficient(const OperatorProxy& op,
			const FactProxy& f) const {
		UNUSED(op);
		UNUSED(f);
		return 0.0;
	}
	virtual double get_time_fact_coefficient(const FactProxy& f) const {
		UNUSED(f);
		return 0.0;
	}
	virtual double get_time_op_coefficient(const OperatorProxy& op) const {
		UNUSED(op);
		return 0.0;
	}
};
}

#endif
