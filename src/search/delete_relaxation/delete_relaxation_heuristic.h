#ifndef DELETE_RELAXATION_HEURISTIC_H
#define DELETE_RELAXATION_HEURISTIC_H

#include "./constraints/delete_relaxation_constraints_generator.h"
#include "./constraints/delete_relaxation_constraints_original.h"
#include "delete_relaxation_model.h"
#include "delete_relaxation_variables.h"
#include "../plan_extractable_heuristic.h"
#include "../lp_solver.h"
#include "../task_proxy.h"
#include <string>
#include <sstream>

#include <memory>
#include <vector>
#include <map>

using std::string;
using std::endl;
using std::map;

class Options;

//typedef map<FactProxyKey, LPVariableWithIndex> FirstAchieverMap;
//typedef map<FactProxyKey, std::set<int>> FactAchieverMap;

namespace DeleteRelaxation {
typedef std::vector<const GlobalOperator *> Plan;

class DeleteRelaxationHeuristic: public PlanExtractableHeuristic {
protected:
	LPSolver lp_solver;
	DeleteRelaxationObjective* objective;
	DeleteRelaxationModel* model;
	DeleteRelaxationVariables* lp_variables;
	std::vector<ConstraintsGenerator *> constraints_generators;

	void add_state_to_constraints(const State&, double);
	void add_variable(LPVariableWithIndex&);
	void print_variables(const State&);
	void print_plan();
	bool fact_in_init_or_goal(const State&, int, int);

	virtual void initialize();
	virtual int compute_heuristic(const GlobalState &global_state)
override	;
	virtual int compute_heuristic(const State &state);
	virtual double compute_heuristic_double(const State&);
	virtual void build_variables(double, const State&);
	virtual void build_constraints(double, const State&);

public:
	explicit DeleteRelaxationHeuristic(const Options &opts);
	~DeleteRelaxationHeuristic();

//	void build_propositions(const TaskProxy &task_proxy);
	void add_constraints(std::vector<LPConstraint> &constraints, double infinity);
	void add_constraints(LPConstraint&, const State&, double);

	virtual void solve();

	bool update_constraints(const State &,
			LPSolver &);

	virtual Plan extract_plan();
	virtual double compute_heuristic_double(const GlobalState&);
};}

#endif
