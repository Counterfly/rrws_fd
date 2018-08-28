#ifndef DELETE_RELAXATION_VARIABLES_H
#define DELETE_RELAXATION_VARIABLES_H

#include "delete_relaxation_model.h"

#include "../lp_solver.h"
#include "../task_proxy.h"
#include <string>
#include <sstream>

#include <memory>
#include <vector>
#include <set>
#include <map>
#include <stdio.h>

using std::string;
using std::endl;
using std::map;
using std::vector;
using std::set;

#define UNUSED(expr) do { (void)(expr); } while (0)

class LPVariableWithIndex {
public:
	double lower_bound, upper_bound, objective_coefficient;
	bool is_integer;
	int index;

	LPVariableWithIndex(int index, LPVariable variable);
	LPVariableWithIndex(int, double, double, double, bool);
	LPVariableWithIndex() :
			lower_bound(0), upper_bound(0), objective_coefficient(0), is_integer(
					false), index(-1) {
	}
	LPVariableWithIndex(const LPVariableWithIndex& other) :
			lower_bound(other.lower_bound), upper_bound(other.upper_bound), objective_coefficient(
					other.objective_coefficient), is_integer(other.is_integer), index(
					other.index) {
	}

	int get_lower_bound() {
		return get_lpvariable().lower_bound;
	}
	int get_upper_bound() {
		return get_lpvariable().upper_bound;
	}
	int get_objective_coefficient() {
		return get_lpvariable().objective_coefficient;
	}
	bool get_is_integer() {
		return get_lpvariable().is_integer;
	}
	LPVariableWithIndex& operator=(const LPVariableWithIndex &other) {
		this->index = other.index;
		this->lower_bound = other.lower_bound;
		this->upper_bound = other.upper_bound;
		this->objective_coefficient = other.objective_coefficient;
		this->is_integer = other.is_integer;
		return *this;
	}

	~LPVariableWithIndex();
	LPVariable get_lpvariable() {
		return LPVariable(lower_bound, upper_bound, objective_coefficient,
				is_integer);
	}
};

class LPVariableProxy {
public:
	string details;

	LPVariableProxy();
	LPVariableProxy(FactProxy f, string = "");
	LPVariableProxy(OperatorProxy op, string = "");
	LPVariableProxy(OperatorProxy op, FactProxy f);
	~LPVariableProxy() {
	}
	;
	string get_details() {
		return details;
	}
private:
	string fact_details(FactProxy fact) {
		std::ostringstream oss;
		oss << "Fact[" << fact.get_name() << " = " << fact.get_value() << "]";
		return oss.str();
	}
	string op_details(OperatorProxy op) {
		std::ostringstream oss;
		oss << "Operator[" << op.get_name();
//		string sep = "";
//		oss << endl << "\tPRE={";
//
//		for (FactProxy f : op.get_preconditions()) {
//			oss << sep << fact_details(f);
//			sep = ", ";
//		}
//		oss << "}" << endl << "\tPOST={";
//		sep = "";
//		for (EffectProxy ef : op.get_effects()) {
//			FactProxy f = ef.get_fact();
//			oss << sep << fact_details(f);
//			sep = ", ";
//		}
//		oss << "}" << endl;
		return oss.str();
	}
};

class FactProxyKey {
	int var_id;
	int value;
public:
	FactProxyKey(int var_id, int value) :
			var_id(var_id), value(value) {
	}
	FactProxyKey(const FactProxyKey &other) :
			var_id(other.var_id), value(other.value) {
	}
	FactProxyKey(const FactProxy &other) :
			var_id(other.get_variable().get_id()), value(other.get_value()) {
	}
	~FactProxyKey() = default;

	VariableProxy get_variable_id() const;

	int get_id() const {
		return var_id;
	}
	int get_value() const {
		return value;
	}

	bool operator==(const FactProxyKey &other) const {
		return var_id == other.var_id && value == other.value;
	}

	bool operator!=(const FactProxyKey &other) const {
		return !(*this == other);
	}
	bool operator<(const FactProxyKey &other) const {
		if (var_id < other.var_id)
		return true;
		else if (var_id > other.var_id)
		return false;
		else {
			if (value < other.value)
			return true;
			else
			return false;
		}
	}
};

namespace DeleteRelaxation {
typedef map<FactProxyKey, LPVariableWithIndex> FirstAchieverMap;
//class DeleteRelaxationModel;
extern map<FactProxyKey, bool> relevant;

bool contains_fact_proxy(const PreconditionsProxy& preconditions,
		const FactProxy& fact);
bool inverse(const OperatorProxy& op1, const OperatorProxy& op2);

class DeleteRelaxationVariables {
private:
	// Prevent copying and assignment
	DeleteRelaxationVariables(const DeleteRelaxationVariables&) {
	}
	DeleteRelaxationVariables& operator=(const DeleteRelaxationVariables&) {
		return *this;
	}
//	static DeleteRelaxationVariables* _instance;

public:
	DeleteRelaxationVariables(const Options&);
	DeleteRelaxationVariables(DeleteRelaxationModel*);
	DeleteRelaxationModel* model;
	std::vector<LPVariable> variables;
	std::vector<LPVariableProxy> variable_proxies; // To inspect solution
	std::vector<LPConstraint> constraints;

	//vector[op_id][precondition_of_op_id] = set<op_ids> that are inverse actions of op_id with add-effect 'precondition_of_op_id'
	vector<map<FactProxyKey, set<int>>> inverse_actions_eff; // Inverse actions with add effect

	// vector[op_id] = set of operator_ids which are inverse actions of op_id
	vector<map<FactProxyKey, set<int>>> inverse_actions_pre;// Inverse actions

	int lp_variable_index;

	// Variable with value 1 (value assigned is variable index)
	LPVariableWithIndex LPONE;

	// Variable with some aribtarily small value.
	double EPSILON = 0.5;
	LPVariableWithIndex LPEPSILON;

	//vector[var_id][value] = {ops, that, achieve, fact, var_id=value}
	std::vector<std::vector<std::set<int>>>fact_achievers;
	//	FactAchieverMap fact_achievers; // Easier way to reference which Ops achieve certain facts

	// vector[op_id][(var_id],var_value)] = 0/1
	std::vector<FirstAchieverMap> first_achiever;

	// map[fact] = LPVariable
	map<FactProxyKey, LPVariableWithIndex> facts_to_achieve;// Goals - Initial State

	//vector[var_id][var_value] = 0..|OP|
	std::vector<std::vector<LPVariableWithIndex>> time_facts;

	//vector[op_id] = 0..|OP|
	std::vector<LPVariableWithIndex> time_ops;

	// vector[var_id][value] = LPVariableIndex;
	// Used Fact Variables
	std::vector<std::vector<LPVariableWithIndex>> usedFacts;

	// same as used facts
	std::vector<std::vector<LPVariableWithIndex>> initialFacts;

	// Used Action Variables
	std::vector<LPVariableWithIndex> usedOps;

//	void reset() {
//		variables.clear();
//		variable_proxies.clear();
//		constraints.clear();
//
//		fact_achievers.clear();
//		facts_to_achieve.clear();
//		first_achiever.clear();
//		time_facts.clear();
//		time_ops.clear();
//		usedFacts.clear();
//		initialFacts.clear();
//		usedOps.clear();
//
//		lp_variable_index = 2; //ONE has index 0, EPSILON has index 1
//		variables.push_back(LPONE.get_lpvariable());
//		variable_proxies.push_back(LPVariableProxy());
//		variables.push_back(LPEPSILON.get_lpvariable());
//		variable_proxies.push_back(LPVariableProxy());
//	}

	void initialize(const TaskProxy&);

	void clear_constraints() {
		constraints.clear();
	}

	bool is_initial_fact_integer() const {
		return model->is_initial_fact_integer();
	}

	bool is_used_fact_integer() const {
		return model->is_used_fact_integer();
	}

	bool is_used_op_integer() const {
		return model->is_used_op_integer();
	}

	bool is_first_achiever_integer() const {
		return model->is_first_achiever_integer();
	}

	bool is_time_fact_integer() const {
		return model->is_time_fact_integer();
	}

	bool is_time_op_integer() const {
		return model->is_time_op_integer();
	}

	virtual void print_variables(const vector<double>&, const State&, const TaskProxy&);
};}

#endif
