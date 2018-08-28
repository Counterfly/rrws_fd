#include "ehc_mark.h"

#include "const_evaluator_mark.h"
#include "global_operator.h"
#include "plugin.h"
#include "pref_evaluator.h"
#include "successor_generator.h"
#include "utilities.h"
#include "plateau_statistics.h"
#include "scalar_evaluator.h"

#include "const_evaluator_mark.h"
#include "fetch_type.h"
#include "fetch_type_lazy.h"

using namespace std;

#include <iostream>
#include <map>

std::ostream& operator<<(std::ostream& out, const PreferredUsage pref) {
	static std::map<PreferredUsage, std::string> strings;
	if (strings.size() == 0) {
#define INSERT_ELEMENT(p) strings[p] = #p
		INSERT_ELEMENT(PreferredUsage::PRUNE_BY_PREFERRED);
		INSERT_ELEMENT(PreferredUsage::RANK_PREFERRED_FIRST);
#undef INSERT_ELEMENT
	}

	return out << strings[pref];
}

EHC::EHC(const Options &opts) :
		SearchEngine(opts), evaluator(opts.get<ScalarEvaluator *>("eval")), heuristic(
				opts.get<Heuristic *>("h")), preferred_operator_heuristics(
				opts.get_list<Heuristic *>("preferred")), preferred_usage(
				PreferredUsage(opts.get_enum("preferred_usage"))), current_eval_context(
				g_initial_state(), &statistics), num_ehc_phases(0), last_num_expanded(
				-1), num_plateau_phases(0), num_non_plateau_phases(0), num_plateau_recursions(
				0), num_actions_taken(0), plateau_statistics(), fetch(
				opts.get<FetchType *>("fetch")) {

	heuristics.insert(preferred_operator_heuristics.begin(),
			preferred_operator_heuristics.end());
	heuristics.insert(heuristic);
	use_preferred = find(preferred_operator_heuristics.begin(),
			preferred_operator_heuristics.end(), heuristic)
			!= preferred_operator_heuristics.end();

	cout << "Fetch is " << fetch->to_string() << endl;

	 if (use_preferred && preferred_usage == PreferredUsage::PRUNE_BY_PREFERRED) {
		 cout << "Using std scalar open list" << endl;
		 open_list = new StandardScalarOpenList<OpenListEntryEHC>(evaluator, false);
	 } else if (use_preferred && preferred_usage == PreferredUsage::RANK_PREFERRED_FIRST) {
		 vector<ScalarEvaluator *> evals { evaluator, new PrefEvaluator };
		 cout << "using tie breaking open list" << endl;
		 open_list = new TieBreakingOpenList<OpenListEntryEHC>(evals, false, true);
	 }
	 else {
		 // BFS
		 open_list = new BucketOpenList<OpenListEntryEHC>(false, evaluator);
	 }
}

EHC::~EHC() {
	delete fetch; //temp
	delete evaluator;
	delete open_list;
	for (unsigned int i = 0; i < plateau_statistics.size(); ++i)
		delete plateau_statistics.at(i);
}

void EHC::reach_state(const GlobalState &parent, const GlobalOperator &op,
		const GlobalState &state) {
	for (Heuristic *heur : heuristics) {
		heur->reach_state(parent, op, state);
	}
}

void EHC::initialize() {
	assert(heuristic);
	cout << "Conducting enforced hill-climbing search, (real) bound = " << bound
			<< endl;
	if (use_preferred) {
		cout << "Using preferred operators for "
				<< (preferred_usage == PreferredUsage::RANK_PREFERRED_FIRST ?
						"ranking successors" : "pruning") << endl;
	}

	bool dead_end = current_eval_context.is_heuristic_infinite(heuristic);
	statistics.inc_evaluated_states();
	print_initial_h_values(current_eval_context);

	if (dead_end) {
		cout << "Initial state is a dead end, no solution" << endl;
		if (heuristic->dead_ends_are_reliable())
			exit_with(EXIT_UNSOLVABLE);
		else
			exit_with(EXIT_UNSOLVED_INCOMPLETE);
	}

	SearchNode node = search_space.get_node(current_eval_context.get_state());
	node.open_initial();
//	current_eval_context = EvaluationContext(current_eval_context.get_cache(),
//			node.get_g(), current_eval_context.is_preferred(),
//			&current_eval_context.get_statistics(),
//			current_eval_context.get_calculate_preferred()); // Make it so initial state has g-cost of 0 instead of -1
}

vector<const GlobalOperator *> EHC::get_successors(
		EvaluationContext &eval_context) {

	//PlateauStatistics* pstats = &plateau_statistics.back();
	//cout << "RANK, PRUNE = " << (int) PreferredUsage::PRUNE_BY_PREFERRED << ", " << (int) PreferredUsage::RANK_PREFERRED_FIRST << endl;
	vector<const GlobalOperator *> ops;
	if (!use_preferred
			|| preferred_usage == PreferredUsage::RANK_PREFERRED_FIRST) {
		g_successor_generator->generate_applicable_ops(eval_context.get_state(),
				ops);

		// Mark preferred operators.
		if (use_preferred
				&& preferred_usage == PreferredUsage::RANK_PREFERRED_FIRST) {
			for (const GlobalOperator *op : ops) {
				op->unmark();
			}
			for (Heuristic *pref_heuristic : preferred_operator_heuristics) {
				const vector<const GlobalOperator *> &pref_ops =
						eval_context.get_preferred_operators(pref_heuristic);
				for (const GlobalOperator *op : pref_ops) {
					op->mark();
				}
			}
		}
	} else {
		// PRUNE BY PREFERRED OPERATORS
		for (Heuristic *pref_heuristic : preferred_operator_heuristics) {
			const vector<const GlobalOperator *> &pref_ops =
					eval_context.get_preferred_operators(pref_heuristic);
			for (const GlobalOperator *op : pref_ops) {
				if (!op->is_marked()) {
					op->mark();
					ops.push_back(op);
				}
			}
		}
	}
	statistics.inc_expanded();
	statistics.inc_generated_ops(ops.size());
	return ops;
}

void EHC::expand(EvaluationContext &eval_context, int relative_g, int depth) {
	for (const GlobalOperator *op : get_successors(eval_context)) {
		int new_g = relative_g + get_adjusted_cost(*op); // g-cost according to cost_type (relative to current decision point/plateau)...d is reset every time we find/accept a better state
		// Use fetch
		EvaluationContext context_real_g = fetch->expand_state(eval_context,
				*op, cost_type);
		EvaluationContext new_eval_context(context_real_g.get_cache(), new_g,
				op->is_marked(), &statistics);

//		OpenListEntryEHC entry = make_pair(
//				make_pair(new_eval_context, depth),
//				make_pair(new_g, op));

		OpenListEntryEHC entry(eval_context.get_state().get_id(),
				new_eval_context, depth + 1, new_g, op);
		open_list->insert(new_eval_context, entry);
		op->unmark();
	}

	SearchNode node = search_space.get_node(eval_context.get_state());
	node.close();
}

SearchStatus EHC::step() {
	last_num_expanded = statistics.get_expanded();
	search_progress.check_progress(current_eval_context);

	if (check_goal_and_set_plan(current_eval_context.get_state())) {
		// Make sure current Plateau is real
		return SOLVED;
	}

	PlateauStatistics* pstats = new PlateauStatistics(num_actions_taken);
	plateau_statistics.push_back(pstats);

	expand(current_eval_context, 0, 0);

	SearchStatus status = ehc();

	if (!in_plateau()) {
		// If current 'hill climb' was successful, delete last plateau stats
		pstats = plateau_statistics.back();
		plateau_statistics.pop_back();
		delete pstats;
	}
	return status;
}

SearchStatus EHC::ehc() {
	PlateauStatistics* plateau_stats = plateau_statistics.back();
	while (!open_list->empty()) {
		OpenListEntryEHC entry = open_list->remove_min();
		EvaluationContext open_list_context = entry.open_list_context;
		StateID parent_state_id = entry.parent_state_id;
//		StateId state_id = entry.open_list_state_id;
		int depth = entry.depth;
		int d = entry.g_value;
		const GlobalOperator *last_op = entry.op;

		GlobalState parent_state = g_state_registry->lookup_state(
				parent_state_id);
		SearchNode parent_node = search_space.get_node(parent_state);

		//if (parent_node.get_real_g() + last_op->get_cost() >= bound)
		//continue;

		EvaluationContext eval_context = fetch->evaluate_state(
				open_list_context, *last_op, cost_type);
		SearchNode node = search_space.get_node(eval_context.get_state());

//		statistics.inc_generated();

		if (node.is_new()) {
//			EvaluationContext eval_context(state, &statistics); // Eval Context of successor
//			reach_state(parent_state, *last_op, state);
			statistics.inc_evaluated_states();
			plateau_stats->inc_num_states(depth);

			if (eval_context.is_heuristic_infinite(heuristic)) {
				node.mark_as_dead_end();
				statistics.inc_dead_ends();
				//continue;
			}

			int h = eval_context.get_heuristic_value(heuristic); // hvalue of successor
			node.open(parent_node, last_op);

			if (h < current_eval_context.get_heuristic_value(heuristic)) { // if hvalue of successor < hvalue of parent
				++num_ehc_phases;
				if (d_counts.count(d) == 0) {
					d_counts[d] = make_pair(0, 0);
				}
				pair<int, int> &d_pair = d_counts[d];
				d_pair.first += 1;
				d_pair.second += statistics.get_expanded() - last_num_expanded;

				plateau_stats->found_escape(&eval_context, depth);

				// Expand plateau
				if (in_plateau()) {
					expand_plateau();
				}

				current_eval_context = eval_context;
				open_list->clear();
				num_actions_taken += depth; // Increment number of actions by the depth of the new state with lower hvalue
				return IN_PROGRESS;
			} else {
				expand(eval_context, d, depth);
			}
		}
	}
	cout << "No solution - FAILED" << endl;
	return FAILED;
}

void EHC::expand_plateau() {
	PlateauStatistics* plateau_stats = plateau_statistics.back();
	vector<SearchNode> to_reopen;

	int depth, d;
	while (!open_list->empty()) {
		OpenListEntryEHC entry = open_list->remove_min();

		EvaluationContext open_list_context = entry.open_list_context;
		StateID parent_state_id = entry.parent_state_id;
		depth = entry.depth;

		d = entry.g_value;
		const GlobalOperator *last_op = entry.op;

		if (stop_plateau_search(depth)) {
			cout << "Stopping plateau expansion due to stop criteria." << endl;
			open_list->clear();
			break;
		}

		GlobalState parent_state = g_state_registry->lookup_state(
				parent_state_id);
		SearchNode parent_node = search_space.get_node(parent_state);

		//if (parent_node.get_real_g() + last_op->get_cost() >= bound)
		//continue;

		EvaluationContext eval_context = fetch->evaluate_state(
				open_list_context, *last_op, cost_type);
		SearchNode node = search_space.get_node(eval_context.get_state());

//		statistics.inc_generated();
		if (node.is_new()) {
//			EvaluationContext eval_context(state, &statistics); // Eval Context of successor
//			reach_state(parent_state, *last_op, state);
			//statistics.inc_evaluated_states();
			plateau_stats->inc_num_states(depth);

			if (eval_context.is_heuristic_infinite(heuristic)) {
				//node.mark_as_dead_end();
				//statistics.inc_dead_ends();
				//continue;
			}

			int h = eval_context.get_heuristic_value(heuristic); // hvalue of successor

			// Don't affect future search that would have happened without expanding plateau
			// This will ensure any escape found can only be counted once
			node.plateau_open();
			to_reopen.push_back(node);

//			assert(eval_context != current_eval_context);

			if (h < current_eval_context.get_heuristic_value(heuristic)) { // if hvalue of successor < hvalue of parent
				++num_ehc_phases;
				if (d_counts.count(d) == 0) {
					d_counts[d] = make_pair(0, 0);
				}
				pair<int, int> &d_pair = d_counts[d];
				d_pair.first += 1;
				d_pair.second += statistics.get_expanded() - last_num_expanded;

				plateau_stats->found_escape(&eval_context, depth);

//				current_eval_context = eval_context;
//				open_list->clear();
//				num_actions_taken += depth;
//				return IN_PROGRESS;
			} else {
				expand(eval_context, d, depth);
			}
		}
	}

	for (size_t i = 0; i < to_reopen.size(); ++i) {
		to_reopen.at(i).plateau_new();
	}
}

bool EHC::in_plateau() const {
	// if we have not escaped then we need at least X expanded states, if we have escaped then we are in a plateau
	return plateau_statistics.back()->get_depth() >= MIN_PLATEAU_DEPTH;
}

bool EHC::stop_plateau_search(int depth) const {
	bool ret = false;

	int first_escape_depth = plateau_statistics.back()->first_escape_depth();
	ret = ((first_escape_depth > 0)
			&& (depth >= (first_escape_depth + ESCAPE_PLATEAU_DEPTH_INC)));

	return ret;
}

void EHC::print_statistics() const {
	statistics.print_detailed_statistics();

	cout << "EHC phases: " << num_ehc_phases << endl;
	cout << "EHC plateau phases: " << num_plateau_phases << endl;
	cout << "EHC non-plateau phases: " << num_non_plateau_phases << endl;
	cout << "EHC plateau recursions: " << num_plateau_recursions << endl;
	assert(num_ehc_phases != 0);
	cout << "Average expansions per EHC phase: "
			<< static_cast<double>(statistics.get_expanded()) / num_ehc_phases
			<< endl;

	for (auto count : d_counts) {
		int depth = count.first;
		int phases = count.second.first;
		assert(phases != 0);
		int total_expansions = count.second.second;
		cout << "EHC phases of depth " << depth << ": " << phases
				<< " - Avg. Expansions: "
				<< static_cast<double>(total_expansions) / phases << endl;
	}

	// Print Plateau stats
	PlateauStatistics* pstats;
	cout << "================PLATEAU STATS==================" << endl;
	cout << "Number of Plateaus: " << plateau_statistics.size() << endl;
	for (unsigned int i = 0; i < plateau_statistics.size(); ++i) {
		pstats = plateau_statistics.at(i);
		cout << endl << "Plateau " << (i + 1) << "\t";
		pstats->print_statistics();
	}
}

static SearchEngine *_parse(OptionParser &parser) {
	parser.document_synopsis("(Lazy) enforced hill-climbing", "");
	parser.add_option<Heuristic *>("h", "heuristic");
	vector < string > preferred_usages;
	preferred_usages.push_back("PRUNE_BY_PREFERRED");
	preferred_usages.push_back("RANK_PREFERRED_FIRST");
	parser.add_enum_option("preferred_usage", preferred_usages,
			"preferred operator usage", "PRUNE_BY_PREFERRED");
	parser.add_list_option<Heuristic *>("preferred",
			"use preferred operators of these heuristics", "[]");

	parser.add_option<ScalarEvaluator *>("eval",
			"State evaluation used to break ties\n"
					"ConstEvaluatorM is equivalent to Breadth-First Search\n"
					"GEvaluator (default) is equivalent to default EHC search",
			"g");
//	vector<string> fetch_types;
//	fetch_types.push_back("eager");
//	fetch_types.push_back("lazy");
	parser.add_option<FetchType *>("fetch", "state evaluation methodology",
			"lazy");

	SearchEngine::add_options_to_parser(parser);
	Options opts = parser.parse();

	if (parser.dry_run())
		return nullptr;
	else
		return new EHC(opts);
}

static Plugin<SearchEngine> _plugin("ehc_m", _parse);
