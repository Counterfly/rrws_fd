#include "delete_relaxation_heuristic.h"
#include "objectives/delete_relaxation_objective.h"
#include "../plan_not_extractable_exception.h"

#include <stdexcept>
#include <exception>
#include <stdio.h>
#include <string>
#include <map>
#include <utility>

#include "../globals.h"
#include "../global_operator.h"
#include "../lp_solver.h"
#include "../option_parser.h"
#include "../plugin.h"
#include "../successor_generator.h"
#include "../task_tools.h"
#include "../task_proxy.h"

#include <typeinfo>
using namespace std;

#define UNUSED(expr) do { (void)(expr); } while (0)

namespace DeleteRelaxation {

DeleteRelaxationHeuristic::DeleteRelaxationHeuristic(const Options &opts) :
		PlanExtractableHeuristic(opts), lp_solver(
				LPSolverType(opts.get_enum("lpsolver"))), objective(
				opts.get<DeleteRelaxationObjective *>("objective")), model(
				opts.get<DeleteRelaxationModel*>("model")), lp_variables(
				new DeleteRelaxationVariables(model)), constraints_generators(
				opts.get_list<ConstraintsGenerator *>("constraints")) {
//	for (const OperatorProxy& op : task_proxy.get_operators()) {
//		cout << "Operator[" << op.get_name();
//		string sep = "";
//		cout << endl << "\tPRE={";
//
//		for (FactProxy f : op.get_preconditions()) {
//			cout << sep << "Fact[" << f.get_name() << " = " << f.get_value()
//					<< "]";
//			sep = ", ";
//		}
//		cout << "}" << endl;
//		cout << "\tPOST={";
//		sep = "";
//		for (EffectProxy ef : op.get_effects()) {
//			FactProxy f = ef.get_fact();
//			cout << sep << "Fact[" << f.get_name() << " ("
//					<< f.get_variable().get_id() << ") = " << f.get_value()
//					<< "]";
//			sep = ", ";
//		}
//		cout << "}" << endl;
//	}
}

DeleteRelaxationHeuristic::~DeleteRelaxationHeuristic() {
}

/**
 * These constraints assume the LPSolver is configured for >= 'rowsense', thus every constraint will be modeled as
 *	linear function of variables >= 0. '>=' rowsense has value 'G' as of April 2016 for OSI framework
 *
 *	Some exceptions are the initial and goal states which are modeled as: Used(p) >= 1 with domain (0,1)
 */
void DeleteRelaxationHeuristic::build_variables(double infinity,
		const State& initial_state) {
	UNUSED(infinity);

//	lp_variables->reset();

	for (size_t i = 0; i < constraints_generators.size(); ++i) {
		constraints_generators[i]->build_variables(infinity, initial_state,
				task_proxy, *lp_variables, *objective);
	}
}

/**
 * These constraints assume the LPSolver is configured for >= 'rowsense', thus every constraint will be modeled as
 *	linear function of variables >= 0. '>=' rowsense has value 'G' as of April 2016 for OSI framework
 *
 *	Some exceptions are the initial and goal states which are modeled as: Used(p) >= 1 with domain (0,1)
 */
void DeleteRelaxationHeuristic::build_constraints(double infinity,
		const State &initial_state) {
	for (size_t i = 0; i < constraints_generators.size(); ++i) {
		constraints_generators[i]->build_constraints(infinity, initial_state,
				task_proxy, *lp_variables, *model);
	}
}

void DeleteRelaxationHeuristic::initialize() {
	double infinity = lp_solver.get_infinity();
	model->initialize(task_proxy);
	lp_variables->initialize(task_proxy);

	build_variables(infinity, task_proxy.get_initial_state());
	build_constraints(infinity, task_proxy.get_initial_state());
	// Auxiliary Variables to represent objective
// 1. Propositions
// 2. |preconditions(op)| * Used(op)
	/*
	 * This is one approach:
	 * Min(|pre| for each op s.t Used(op)=1). so if multiple ops share a same precondition, it will be counted multiple times
	 *
	 */
//	vector < LPVariable > objective_variables;
//	for (OperatorsProxy op : task_proxy.get_operators()) {
//		for (FactProxy p : op.get_preconditions()) {
//			objective_variables.insert(facts[p.get_variable().get_id()][p.get_value()]);
//		}
//	}
	/*
	 * Min(|PROPS| s.t prop ∈ PROPS if prop is a pre(op) and Used(op)=1
	 * 	this is equivalent to:
	 * Min(used(p) for each p in PROPS) because used(op)=1 only if used(p)=1 forall p ∈ pre(op) and used(p) is not forced to be 1 if used(op)=1 and p ∈ eff(op)
	 */
//	for (VariableProxy p : task_proxy.get_variables()) {
//		for (size_t value = 0; value < var.get_domain_size(); ++value) {
//			objective_variables.insert(facts[p.get_id()][value]);
//		}
//	}
}

void DeleteRelaxationHeuristic::solve() {
	lp_solver.load_problem(LPObjectiveSense::MINIMIZE, lp_variables->variables,
			lp_variables->constraints);
	if (model->is_mip()) {
		lp_solver.solve_mip();
	} else {
		lp_solver.solve();
	}
}

bool DeleteRelaxationHeuristic::update_constraints(const State &state,
		LPSolver &lp_solver) {

	double infinity = lp_solver.get_infinity();
	for (size_t i = 0; i < constraints_generators.size(); ++i) {
		constraints_generators[i]->update_constraints(infinity, state,
				task_proxy, *lp_variables, *model);
	}

	return false;
}

int DeleteRelaxationHeuristic::compute_heuristic(
		const GlobalState &global_state) {
	State state = convert_global_state(global_state);
	return compute_heuristic(state);
}

double DeleteRelaxationHeuristic::compute_heuristic_double(
		const GlobalState &global_state) {
	State state = convert_global_state(global_state);
	return compute_heuristic_double(state);
}

int DeleteRelaxationHeuristic::compute_heuristic(const State& state) {
	double value = compute_heuristic_double(state);
	return ceil(value);
}

double DeleteRelaxationHeuristic::compute_heuristic_double(const State &state) {
	assert(!lp_solver.has_temporary_constraints());
	update_constraints(state, lp_solver);

//	try {
	solve();
//	} catch (...) {
//		cout << "Caught Exception while trying to solve in DelRelHeuristic"
//				<< endl;
//	}

//	cout << "Initial state" << endl;
//	for (size_t var_id = 0; var_id < state.size(); ++var_id) {
//		FactProxy fact = state[var_id];
//		cout << LPVariableProxy(fact).get_details() << endl;
//	}
//	cout << "Goal state" << endl;
//	for (FactProxy goal : task_proxy.get_goals()) {
//		cout << LPVariableProxy(goal).get_details() << endl;
//	}
//	int result;
	double objective_value;
	if (lp_solver.has_solution() && lp_solver.has_optimal_solution()) {
//		double epsilon = 0.01;
		objective_value = lp_solver.get_objective_value();
//		result = ceil(objective_value - epsilon);

		try {
//			print_variables(state);
//			print_plan();

			Plan plan = extract_plan();
			objective_value = objective->h_value(objective_value, plan);
		} catch (PlanNotExtractableException& e) {
//			cout
//					<< "Unable to extract plan, objective value is defaulted to the objective value: "
//					<< e.what() << endl;
		}
	} else {
		objective_value = DEAD_END;
//		print_variables(state);
//		print_plan();
	}

//	cout << "H = " << objective_value << endl << endl;
//	throw std::invalid_argument("temp");
	lp_solver.clear_temporary_constraints();

	return objective_value;
}

bool DeleteRelaxationHeuristic::fact_in_init_or_goal(const State& state,
		int var_id, int value) {

	FactProxy fact = state[var_id];
	if (fact.get_value() == value)
		return true;

	for (FactProxy fact : task_proxy.get_goals()) {
		if (fact.get_variable().get_id() == var_id && fact.get_value() == value)
			return true;
	}
	return false;
}
void DeleteRelaxationHeuristic::print_variables(const State& state) {
	vector<double> vars = lp_solver.extract_solution();
	lp_variables->print_variables(vars, state, task_proxy);
}

typedef std::vector<const GlobalOperator *> Plan;

template<typename T>
vector<size_t> sort_indexes(const vector<T> &v) {

	vector < size_t > indexes(v.size());
	for (size_t i = 0; i < v.size(); ++i) {
		indexes[i] = i;
	}

	std::sort(indexes.begin(), indexes.end(),
			[&v](size_t idx1, size_t idx2) {return v[idx1] < v[idx2];});

	return indexes;
}

Plan DeleteRelaxationHeuristic::extract_plan() {
	if (lp_variables->time_ops.size() <= 0) {
		throw PlanNotExtractableException(
				"Time Operator Variables were not present in ILP Model.");
	}
	vector<double> vars = lp_solver.extract_solution();
	vector<double> time_op_values;

	int start_index = lp_variables->time_ops[0].index;
	for (int i = start_index;
			i < start_index + (int) lp_variables->time_ops.size(); ++i) {
		time_op_values.push_back(vars[i]); //time_ops_values[op_id] = time op_id occurs
	}

	vector < size_t > ordered_op_ids = sort_indexes(time_op_values);

	OperatorsProxy ops = task_proxy.get_operators();

	Plan plan;

//	cout << endl << "=Plan Ordering=" << endl;
	for (int i = 0; i < (int) ordered_op_ids.size(); ++i) {
		int op_id = ordered_op_ids[i];
		const OperatorProxy &op = ops[op_id];
//		cout << op.get_name() //<< "(" << time_op_values[op_id] << ") (used="
//				<< "\t" << vars[lp_variables->usedOps[op_id].index]
//				//<< ")"
//				<< endl;
		if (vars[lp_variables->usedOps[op_id].index] > 0) {
//			const OperatorProxy &op = ops[op_id];
			// op_id is used in plan
			const GlobalOperator* gop = op.get_global_operator();
			plan.push_back(gop);
		}
	}
//
//	cout << endl << "=Ordered by Ops=" << endl;
//	for (size_t op_id = 0; op_id < ops.size(); ++op_id) {
//		const OperatorProxy &op = ops[op_id];
//
//		cout << op.get_name() //<< "(" << time_op_values[op_id] << ") (used="
//				<< "\t" << vars[lp_variables->usedOps[op_id].index]
//				//<< ")"
//				<< endl;
//	}
//
//	cout << endl << "=Ordered by Facts=" << endl;
//	for (VariableProxy var : task_proxy.get_variables()) {
//		int var_id = var.get_id();
//
//		for (int value = 0; value < var.get_domain_size(); ++value) {
//			const FactProxy& fact = var.get_fact(value);
//			cout << fact.get_name() << "\t"
//					<< vars[lp_variables->usedFacts[var_id][value].index]
//					<< endl;
//		}
//	}

	return plan;
}

void DeleteRelaxationHeuristic::print_plan() {
	Plan op_seq = extract_plan();

	cout << "=Plan Order= " << op_seq.size() << endl;
	for (const GlobalOperator* op : op_seq) {
		op->dump();
	}
}

static Heuristic* _parse(OptionParser &parser) {
//	parser.document_synopsis("Delete relaxation heuristic."
//			"Minimizing Number of Propositions"
//			"Minimizing Number of preconditions in used action set");

//    parser.document_language_support("action costs", "supported");
//    parser.document_language_support(
//        "conditional effects",
//        "not supported (the heuristic supports them in theory, but none of "
//        "the currently implemented constraint generators do)");
//    parser.document_language_support(
//        "axioms",
//        "not supported (the heuristic supports them in theory, but none of "
//        "the currently implemented constraint generators do)");
	parser.document_property("admissible", "yes/no depending on objective");
	parser.document_property("consistent", "no");
	parser.document_property("safe", "yes?");
// TODO: prefer operators that are non-zero in the solution.
	parser.document_property("preferred operators",
			"no (can maybe try to add later using First Achievers)");

	add_lp_solver_option_to_parser(parser);
	Heuristic::add_options_to_parser(parser);

	parser.add_option<DeleteRelaxationObjective *>("objective",
			"Objective to Minimize", "deleterelax_min_plan_cost");
	parser.add_option<DeleteRelaxationModel *>("model",
			"Designate Variable types (integer, linear)", "deleterelax_model");
//	parser.add_option<DeleteRelaxationModel *>("lp_variables",
//			"Variable Declarations", "deleterelax_variables");
	parser.add_list_option<ConstraintsGenerator *>("constraints",
			"Constraints for model. Should always include the default.",
			"[deleterelax_og]");

	Options opts = parser.parse();
	if (parser.help_mode())
		return nullptr;
	if (parser.dry_run())
		return nullptr;
	return new DeleteRelaxationHeuristic(opts);
}

static Plugin<Heuristic> _plugin("deleterelax", _parse);
}
