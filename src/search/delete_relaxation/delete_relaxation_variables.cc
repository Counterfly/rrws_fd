#include "delete_relaxation_variables.h"

#include <math.h>
#include <stdio.h>

#include "../option_parser.h"
#include "../plugin.h"

#include <stdio.h>
using std::cout;
using std::endl;

LPVariableWithIndex::LPVariableWithIndex(int index, LPVariable variable) :
		lower_bound(variable.lower_bound), upper_bound(variable.upper_bound), objective_coefficient(
				variable.objective_coefficient), is_integer(
				variable.is_integer), index(index) {
}

LPVariableWithIndex::LPVariableWithIndex(int index, double lower_bound,
		double upper_bound, double objective_coefficient, bool is_integer) :
		lower_bound(lower_bound), upper_bound(upper_bound), objective_coefficient(
				objective_coefficient), is_integer(is_integer), index(index) {
}

LPVariableWithIndex::~LPVariableWithIndex() {
}

LPVariableProxy::LPVariableProxy() :
		details() {
}
LPVariableProxy::LPVariableProxy(FactProxy f, string prefix) :
		details(prefix + fact_details(f)) {
}
LPVariableProxy::LPVariableProxy(OperatorProxy op, string prefix) :
		details(prefix + op_details(op)) {
}
LPVariableProxy::LPVariableProxy(OperatorProxy op, FactProxy f) :
		details(
				string("Operator->Fact:\n") + op_details(op) + "\n"
						+ fact_details(f)) {
}

namespace DeleteRelaxation {
map<FactProxyKey, bool> relevant;

bool contains_fact_proxy(const PreconditionsProxy& preconditions,
		const FactProxy& fact) {
	for (const FactProxy& pre : preconditions) {
		if (pre == fact) {
			return true;
		}
	}
	return false;
}

bool contains_fact_proxy(const EffectsProxy& effects, const FactProxy& fact) {
	for (const EffectProxy& eff : effects) {
		if (eff.get_fact() == fact) {
			return true;
		}
	}
	return false;
}

bool inverse(const OperatorProxy& op1, const OperatorProxy& op2) {

	// add(op1) \subset pre(op2)
	for (const EffectProxy& effect_proxy : op1.get_effects()) {
		const FactProxy& effect = effect_proxy.get_fact();

		if (relevant[effect]
				&& !contains_fact_proxy(op2.get_preconditions(), effect)) {
			return false;
		}
	}

	// add(op2) \subset pre(op1)
	for (const EffectProxy& effect_proxy : op2.get_effects()) {
		const FactProxy& effect = effect_proxy.get_fact();

		if (relevant[effect]
				&& !contains_fact_proxy(op1.get_preconditions(), effect)) {
			return false;
		}
	}

	return true;
}

/*
 * Computes the set of inverse actions of an action 'op' which have fact 'f' as an add-effect.
 */
set<int> inverse_actions_with_effect(const OperatorProxy& op,
		const FactProxy& f, const OperatorsProxy& operators) {

	set<int> inv_ops;
	for (const OperatorProxy& inv_op : operators) {
		if (contains_fact_proxy(inv_op.get_effects(), f)
				&& inverse(op, inv_op)) {
//			cout << "Inverse Actions " << op.get_name() << ", " << f.get_name()
//					<< ", " << inv_op.get_name() << endl;
			inv_ops.insert(inv_op.get_id());
		}
	}
	return inv_ops;
}

/*
 * Computes the set of inverse actions of an action 'op' which have fact 'f' as a precondition.
 */
set<int> inverse_actions_with_precondition(const OperatorProxy& op,
		const FactProxy& f, const OperatorsProxy& operators) {

	set<int> inv_ops;
	for (const OperatorProxy& inv_op : operators) {
		if (contains_fact_proxy(inv_op.get_preconditions(), f)
				&& inverse(op, inv_op)) {
			inv_ops.insert(inv_op.get_id());
		}
	}
	return inv_ops;
}

void DeleteRelaxationVariables::initialize(const TaskProxy& task_proxy) {

	const OperatorsProxy& ops = task_proxy.get_operators();
	// All Goal Facts are relevant
	for (const FactProxy& goal : task_proxy.get_goals()) {
		relevant[goal] = true;
	}

	for (const OperatorProxy& op1 : ops) {
		for (const EffectProxy& effect : op1.get_effects()) {
			const FactProxy& efact = effect.get_fact();

			if (relevant.find(efact) == relevant.end()) { // Only need to search for fact once

				bool fact_relevant = false;
				for (const OperatorProxy& op2 : task_proxy.get_operators()) {
					for (const FactProxy& pre : op2.get_preconditions()) {
						if (pre == efact) {
							fact_relevant = true;
							break;
						}
					}

					if (fact_relevant) {
						break;
					}
				}

//				if (!fact_relevant) {
//					cout << "Fact[" << efact.get_name() << " = "
//							<< efact.get_value() << "] " << "NOT Relevant"
//							<< endl;
//				}
				relevant[efact] = fact_relevant;
			}
		}
	}

	// Calculate Inverse Actions
	if (model->use_inverse_actions()) {
		// Inverse actions with effect
		for (size_t op_id = 0; op_id < ops.size(); ++op_id) {
			const OperatorProxy &op = ops[op_id];
			map<FactProxyKey, set<int>> inverse_map;
			for (const FactProxy& precondition : op.get_preconditions()) {
				inverse_map[precondition] = inverse_actions_with_effect(op,
						precondition, ops);
			}
			inverse_actions_eff.push_back(inverse_map);
		}

		// Inverse actions with precondition
		for (size_t op_id = 0; op_id < ops.size(); ++op_id) {
			const OperatorProxy &op = ops[op_id];
			map<FactProxyKey, set<int>> inverse_map;
			for (const EffectProxy& effect : op.get_effects()) {
				const FactProxy& efact = effect.get_fact();
				inverse_map[efact] = inverse_actions_with_precondition(op,
						efact, ops);
			}
			inverse_actions_pre.push_back(inverse_map);
		}
	}
}

DeleteRelaxationVariables::DeleteRelaxationVariables(const Options& opts) :
		model(opts.get<DeleteRelaxationModel*>("model")), lp_variable_index(0), LPONE(
				lp_variable_index++, 1, 1, 0, false), LPEPSILON(
				lp_variable_index++, EPSILON, EPSILON, 0, false) {
	variables.push_back(LPONE.get_lpvariable());
	variable_proxies.push_back(LPVariableProxy());
	variables.push_back(LPEPSILON.get_lpvariable());
	variable_proxies.push_back(LPVariableProxy());
}

DeleteRelaxationVariables::DeleteRelaxationVariables(
		DeleteRelaxationModel* model) :
		model(model), lp_variable_index(0), LPONE(lp_variable_index++, 1, 1, 0,
				false), LPEPSILON(lp_variable_index++, EPSILON, EPSILON, 0,
				false) {
	variables.push_back(LPONE.get_lpvariable());
	variable_proxies.push_back(LPVariableProxy());
	variables.push_back(LPEPSILON.get_lpvariable());
	variable_proxies.push_back(LPVariableProxy());
}
void DeleteRelaxationVariables::print_variables(const vector<double>& vars,
		const State& initial_state, const TaskProxy& task_proxy) {
	UNUSED(vars);
	UNUSED(task_proxy);
	UNUSED(initial_state);

//	size_t ops_size = task_proxy.get_operators().size();

//	double intpart;
//	double float_rem;
//	for (size_t i = 0; i < vars.size(); ++i) {
//		float_rem = std::modf(vars[i], &intpart);
//		if (float_rem > 0.0 & floam_rem < 1.0) {
//			printf("%.4lf = %.4lf + %.4lf\n", vars[i], intpart, float_rem);
//			cout << variable_proxies[i].get_details() << " == " << vars[i]
//					<< " (" << float_rem << ")" << endl << endl;
//		}
//
//	}

//	cout << "==OPS==" << endl;
//	for (size_t i = 0; i < usedOps.size(); ++i) {
//		int op_index = usedOps[i].index;
//		if (vars[op_index] != 0)
//			cout << variable_proxies[op_index].get_details() << " == "
//					<< vars[op_index] << endl;
//	}
//	cout << "===Objective===" << state[0].get_value() << endl;
//
//	cout << "==Facts not in goal or init==" << endl;
//	for (VariableProxy var : task_proxy.get_variables()) {
//		int var_id = var.get_id();
//		for (int value = 0; value < var.get_domain_size(); ++value) {
//			int index = usedFacts[var_id][value].index;
//			if (!fact_in_init_or_goal(state, var_id, value)
//					&& vars[index] > 0) {
//				cout << variable_proxies[index].get_details() << " == "
//						<< vars[index] << endl;
//			}
//
//		}
//	}
//
	size_t ops_size = task_proxy.get_operators().size();
//
//	cout << "==all vars==" << endl;
//	for (size_t i = 0; i < vars.size(); ++i) {
//		if (variable_proxies[i].get_details().length() > 0) {
//			cout << variable_proxies[i].get_details() << " == " << vars[i]
//					<< endl;
//		}
//	}
//
	cout << "====Used Facts====" << endl;
	for (VariableProxy var : task_proxy.get_variables()) {
		int var_id = var.get_id();
		for (int value = 0; value < var.get_domain_size(); ++value) {
			const FactProxy& fact = var.get_fact(value);
			int index = usedFacts[var_id][value].index;
			//			if (vars[index] > 0) {
			cout << fact.get_name() << "\t" << vars[index] << endl;
			//			}

		}
	}
//
	cout << "==Used Ops==" << endl;
	for (size_t op_id = 0; op_id < ops_size; ++op_id) {
		int index = usedOps[op_id].index;
		if (vars[index] > 0 //&& vars[index] < ops_size
		&& variable_proxies[index].get_details().length() > 0) {
			cout << variable_proxies[index].get_details() << "\t" << vars[index]
					<< endl;
		}
	}
//	if (first_achiever.size() > 0) {
//		cout << "==First Achievers==" << endl;
//		for (size_t op_id = 0; op_id < ops_size; ++op_id) {
//			for (auto it = first_achiever[op_id].begin();
//					it != first_achiever[op_id].end(); it++) {
//				int index = it->second.index;
////				if (vars[index] > 0
////						&& variable_proxies[index].get_details().length() > 0) {
//				cout << variable_proxies[index].get_details() << "\t"
//						<< vars[index] << endl;
////				}
//			}
//		}
//	}

//	if (time_facts.size() > 0) {
//		cout << endl << "==Time Facts==" << endl;
//		for (VariableProxy var : task_proxy.get_variables()) {
//			int var_id = var.get_id();
//			for (int value = 0; value < var.get_domain_size(); ++value) {
//				int index = time_facts[var_id][value].index;
////				if (vars[index] > 0 && vars[index] < ops_size) {
//				if (vars[usedFacts[var_id][value].index] > 0) {
//					cout << variable_proxies[index].get_details() << endl
//							<< "   == " << vars[index] << " (used="
//							<< vars[usedFacts[var_id][value].index] << ")"
//							<< endl;
//				}
////				}
//
//			}
//		}
//	}
	//
//	cout << "==Time Ops==" << endl;
//	assert(ops_size == time_ops.size());
//	if (time_ops.size() > 0) {
//		for (size_t op_id = 0; op_id < ops_size; ++op_id) {
//			int index = time_ops[op_id].index;
////			if (vars[time_ops[op_id].index] > 0) {
//			if (vars[usedOps[op_id].index] > 0) {
////				if (vars[usedOps[op_id].index] < ops_size) {
//				cout << variable_proxies[index].get_details() << endl << " == "
//						<< vars[index] << "(used=" << vars[usedOps[op_id].index]
//						<< ")" << endl;
////				}
//			}
////			}
//		}
//	}
//
//	cout << "====Initial Facts====" << endl;
//	for (VariableProxy var : task_proxy.get_variables()) {
//		int var_id = var.get_id();
//		for (int value = 0; value < var.get_domain_size(); ++value) {
//			int index = initialFacts[var_id][value].index;
//			if (vars[index] > 0) {
//				cout << variable_proxies[index].get_details() << " == "
//						<< vars[index] << endl;
//			}
//
//		}
//	}

	cout << "== Used Time ==" << endl;
	OperatorsProxy ops = task_proxy.get_operators();
	for (size_t op_id = 0; op_id < ops_size; ++op_id) {
//		if (vars[index] > 0 //&& vars[index] < ops_size
//		&& variable_proxies[index].get_details().length() > 0) {
		//cout << variable_proxies[index].get_details() << "\t" << vars[index] << endl;
		const OperatorProxy& op = ops[op_id];
		cout << op.get_name() << "\t";
		if (usedOps.size() > 0) {
			int index = usedOps[op_id].index;
			cout << vars[index];
		} else {
			cout << -1;
		}
		cout << "\t";

		if (time_ops.size() > 0) {
			int time_index = time_ops[op_id].index;
			cout << vars[time_index];
		} else {
			cout << -1;
		}
		cout << endl;
	}
	cout << "|OPS| = " << ops_size << endl;
}

static DeleteRelaxationVariables* _parse(OptionParser &parser) {
	Options opts = parser.parse();
	if (parser.help_mode())
		return nullptr;
	if (parser.dry_run())
		return nullptr;
	return new DeleteRelaxationVariables(opts);
}

static Plugin<DeleteRelaxationVariables> _plugin("deleterelax_variables",
		_parse);
}
