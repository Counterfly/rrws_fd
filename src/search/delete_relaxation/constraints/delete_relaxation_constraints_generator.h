#ifndef DELETE_RELAXATION_CONSTRAINTS_GENERATOR_H
#define DELETE_RELAXATION_CONSTRAINTS_GENERATOR_H

#define UNUSED(expr) do { (void)(expr); } while (0)

#include "../objectives/delete_relaxation_objective.h"
#include "../delete_relaxation_variables.h"
#include "../delete_relaxation_model.h"
#include "../../lp_solver.h"
#include "../../task_proxy.h"

#include <set>
#include <sstream>
#include <string>
#include <map>

using std::endl;
using std::string;
using std::vector;
using std::map;

namespace DeleteRelaxation {
class DeleteRelaxationObjective;

class ConstraintsGenerator {

public:
	ConstraintsGenerator() {
	}
	~ConstraintsGenerator() {
	}

	virtual void build_variables(double, const State&, const TaskProxy&,
			DeleteRelaxationVariables&, const DeleteRelaxationObjective&) = 0;
	virtual void build_constraints(double, const State&, const TaskProxy&,
			DeleteRelaxationVariables&, const DeleteRelaxationModel&) = 0;
	virtual void update_constraints(double, const State&, const TaskProxy&,
			DeleteRelaxationVariables&, const DeleteRelaxationModel&);
};
} // END DelRel namespace

#endif
