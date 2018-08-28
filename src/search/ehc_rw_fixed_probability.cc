#include "ehc_rw_fixed_probability.h"

#include "g_evaluator.h"
#include "global_operator.h"
#include "plugin.h"
#include "pref_evaluator.h"
#include "successor_generator.h"
#include "utilities.h"
#include "restart_strategy.h"

#include <algorithm>  // for random_shuffle
#include <cstdint>
#include "stdio.h"	// for NULL
#include "stdlib.h" // for rand() and srand
#include "time.h"	// for time

#include <sys/time.h>

using namespace std;

EnforcedHillClimbingSearchRWFixedProbability::EnforcedHillClimbingSearchRWFixedProbability(
    const Options &opts)
    : SearchEngine(opts),
      g_evaluator(new GEvaluator()),
      heuristic(opts.get<Heuristic *>("h")),
      preferred_operator_heuristics(opts.get_list<Heuristic *>("preferred")),
      preferred_usage(PreferredUsage(opts.get_enum("preferred_usage"))),
      restart_probability(opts.get<double>("prob")),
      probability_preferred(opts.get<double>("pref_prob")),
      num_execution_samples(opts.get<int>("runs")),
	  stats(num_execution_samples),
      current_eval_context(g_initial_state(), &statistics),
      num_ehc_phases(0),
      last_num_expanded(-1) {
    heuristics.insert(preferred_operator_heuristics.begin(),
                      preferred_operator_heuristics.end());
    heuristics.insert(heuristic);
    use_preferred = find(preferred_operator_heuristics.begin(),
                         preferred_operator_heuristics.end(), heuristic) !=
                    preferred_operator_heuristics.end();

    cout << "===Preferred Usage===" << use_preferred << " , " << preferred_usage << endl << "===---------===" << endl;
    if (!use_preferred || preferred_usage == PreferredUsage::PRUNE_BY_PREFERRED) {
        open_list = new StandardScalarOpenList<OpenListEntryEHC>(g_evaluator, false);
    } else {
        vector<ScalarEvaluator *> evals {
            g_evaluator
        };
        if (preferred_usage == PreferredUsage::RANK_PREFERRED_FIRST) {
			evals.push_back(new PrefEvaluator);
		}
		// else prefusage == DO_NOTHING
        open_list = new TieBreakingOpenList<OpenListEntryEHC>(evals, false, true);
    }

    if (preferred_usage == PreferredUsage::RANK_PREFERRED_FIRST) {
       	cout << "ILOGICAL: Ranking by preferred does nothing because of randomizing operators" << endl;
       	cout << "Ranking only 'influences' hill-climbing (does not prune as all ops are randomized anyway) but is not used in local minimia" << endl;
       	//exit(-1);
    }

    if (!use_preferred && probability_preferred != -1) {
		cout << "ILOGICAL: preferred operator heuristic NOT specified but wanting to use a biased probability distribution" << endl;
		exit(-1);
	}

    struct timeval time;
	gettimeofday(&time,NULL);

	// microsecond has 1 000 000
	// Assuming you did not need quite that accuracy
	// Also do not assume the system clock has that accuracy.
	srand((time.tv_sec * 1000) + (time.tv_usec / 1000));
}

EnforcedHillClimbingSearchRWFixedProbability::~EnforcedHillClimbingSearchRWFixedProbability() {
    delete g_evaluator;
    delete open_list;
}

void EnforcedHillClimbingSearchRWFixedProbability::reach_state(
    const GlobalState &parent, const GlobalOperator &op, const GlobalState &state) {
    for (Heuristic *heur : heuristics) {
        heur->reach_state(parent, op, state);
    }
}

void EnforcedHillClimbingSearchRWFixedProbability::save_plan_if_necessary() const {
	bool multiple = num_execution_samples > 1;
	for (int i = 0; i < num_execution_samples; ++i) {
		if (stats[i].get_found_solution()) {
			save_plan(stats[i].get_plan(), multiple);
		}
	}
}

void EnforcedHillClimbingSearchRWFixedProbability::initialize() {
    assert(heuristic);
    cout << "Conducting enforced hill-climbing search, (real) bound = "
         << bound << endl;
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

    // Re-initialize
    clear_plan();
    num_ehc_phases = 0;
    d_counts.clear();

    SearchNode node = search_space.get_node(current_eval_context.get_state());
    node.open_initial();
}

vector<const GlobalOperator *> EnforcedHillClimbingSearchRWFixedProbability::get_successors(
    EvaluationContext &eval_context) {
    vector<const GlobalOperator *> ops;
    if (!use_preferred || preferred_usage == PreferredUsage::RANK_PREFERRED_FIRST || preferred_usage == PreferredUsage::DO_NOTHING) {
        g_successor_generator->generate_applicable_ops(eval_context.get_state(), ops);

        // Mark preferred operators.
        if (use_preferred &&
            preferred_usage == PreferredUsage::RANK_PREFERRED_FIRST) {
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

    // Randomize ops
    std::random_shuffle(ops.begin(), ops.end());
    return ops;
}

vector<const GlobalOperator *> EnforcedHillClimbingSearchRWFixedProbability::get_biased_successors(
    EvaluationContext &eval_context) {
    vector<const GlobalOperator *> ops;
	g_successor_generator->generate_applicable_ops(eval_context.get_state(), ops);

	std::set<const GlobalOperator *> pref_ops;
    if (use_preferred) {
        for (Heuristic *pref_heuristic : preferred_operator_heuristics) {
            const vector<const GlobalOperator *> &pref_ops1 =
                eval_context.get_preferred_operators(pref_heuristic);
            //cout << "pref heur = " << pref_heuristic->get_description() << " num pref ops = " << pref_ops1.size() << endl;
            for (const GlobalOperator *op : pref_ops1) {
            	pref_ops.insert(op);
            }
        }
    }

    std::set<const GlobalOperator *> non_pref_ops;
    for (const GlobalOperator * op : ops) {
    	if (pref_ops.find(op) == pref_ops.end()) {
    		non_pref_ops.insert(op);
    	}
    }

    statistics.inc_expanded();
    statistics.inc_generated_ops(ops.size());


    if (probability_preferred != -1) {
		ops.clear();

		// Before doing randomization, see if one list is empty which forces deterministic choice
		if (pref_ops.size() == 0) {
			//cout << "Pref Operators is empty" << endl;
			for (const GlobalOperator * op : non_pref_ops) {
				ops.push_back(op);
			}
		}
		else if (non_pref_ops.size() == 0) {
			//cout << "NonPref Operators is empty" << endl;
			for (const GlobalOperator * op : pref_ops) {
				ops.push_back(op);
			}
		}
		else {
			// Both operator types exist, randomly choose between the two sets
			//int r = (rand() % 100);
			double r = this->get_probability();
			//cout << "randomed...." << r << endl;
			if (r < probability_preferred) {
				//cout << "randoming among preferred" << endl;
				for (const GlobalOperator * op : pref_ops) {
					ops.push_back(op);
				}
			}
			else {
				//cout << "randoming among non_pref" << endl;
				for (const GlobalOperator * op : non_pref_ops) {
					ops.push_back(op);
				}
			}
		}
		/*
		// Bias operators for appropriate distribution
		ops.clear();

		for (int i = 0; i < this->instances_non_preferred; ++i){
			for (const GlobalOperator * op : non_pref_ops) {
				ops.push_back(op);
			}
		}
		cout << "instances-non-pref = " << this->instances_non_preferred << ", num non-pref ops = " << non_pref_ops.size() << ", ops size = " << ops.size() << endl;

		for (int i = 0; i < this->instances_preferred; ++i){
			for (const GlobalOperator * op : pref_ops) {
				ops.push_back(op);
			}
		}
		*/
		//cout << "num pref ops = " << pref_ops.size() << ", new ops size = " << ops.size() << endl;
    }

    // Randomize ops
    std::random_shuffle(ops.begin(), ops.end());
    return ops;
}

void EnforcedHillClimbingSearchRWFixedProbability::expand(EvaluationContext &eval_context, int d) {
    for (const GlobalOperator *op : get_successors(eval_context)) {
        int new_d = d + get_adjusted_cost(*op);	// 0/1 if pref op
        OpenListEntryEHC entry = make_pair(
            eval_context.get_state().get_id(), make_pair(new_d, op));
        EvaluationContext new_eval_context(
            eval_context.get_cache(), new_d, op->is_marked(), &statistics);

        open_list->insert(new_eval_context, entry);
        op->unmark();
    }

    SearchNode node = search_space.get_node(eval_context.get_state());
    node.close();
}

bool EnforcedHillClimbingSearchRWFixedProbability::check_goal_and_set_plan(const GlobalState &state) {
    if (test_goal(state)) {
        cout << "Solution found!" << endl;
        Plan plan;
        //search_space.trace_path(state, plan);
        // extract plan

        GlobalState current_state = g_initial_state();
        // Plan = vector<const GlobalOperator *>
        while (!(current_state == state)) {
        	vector<const GlobalOperator*> ops = used_actions.at(current_state.get_id());
        	for (const GlobalOperator* op : ops) {
        		plan.push_back(op);
        		current_state = g_state_registry->get_successor_state(current_state, *op);
        	}
        }

        set_plan(plan);
        return true;
    }
    return false;
}

void EnforcedHillClimbingSearchRWFixedProbability::search() {
	for (int i = 0; i < num_execution_samples; ++i) {
		initialize();
		SearchStatus status = IN_PROGRESS;
		timer = new CountdownTimer(this->get_max_time());	// Use as private instance variable so don't have to pass it through function
		while (status == IN_PROGRESS) {
			status = step();
			if (timer->is_expired()) {
				// Timer is done through system (python scripts check call.py, limits.py)
				// Timer will always be inf....stupid
				cout << "Time limit reached. Abort search." << endl;
				status = TIMEOUT;
				break;
			}
		}

		cout << "Actual search time: " << *timer << " [t=" << g_timer << "]" << endl;
		delete timer;


		//save_plan_if_necessary();
		statistics.set_plan(get_plan());
		stats[i] = statistics;
		statistics = SearchStatistics();	// Clear search statistics

		current_eval_context = EvaluationContext(g_initial_state(), &statistics);
	}
}


SearchStatus EnforcedHillClimbingSearchRWFixedProbability::step() {
    last_num_expanded = statistics.get_expanded();
    search_progress.check_progress(current_eval_context);

    if (check_goal_and_set_plan(current_eval_context.get_state())) {
        return SOLVED;
    }

    expand(current_eval_context, 0);
    return ehc();
}

SearchStatus EnforcedHillClimbingSearchRWFixedProbability::ehc() {
    while (!open_list->empty()) {
        OpenListEntryEHC entry = open_list->remove_min();
        StateID parent_state_id = entry.first;
        int d = entry.second.first;
        const GlobalOperator *last_op = entry.second.second;

        GlobalState parent_state = g_state_registry->lookup_state(parent_state_id);
        SearchNode parent_node = search_space.get_node(parent_state);

        if (parent_node.get_real_g() + last_op->get_cost() >= bound)
            continue;

        GlobalState state = g_state_registry->get_successor_state(parent_state, *last_op);
        statistics.inc_generated();

        SearchNode node = search_space.get_node(state);

        if (node.is_new()) {
            EvaluationContext eval_context(state, &statistics);	// Eval Context of successor
            reach_state(parent_state, *last_op, state);
            statistics.inc_evaluated_states();

            if (eval_context.is_heuristic_infinite(heuristic)) {
                node.mark_as_dead_end();
                statistics.inc_dead_ends();
                continue;
            }

            int h = eval_context.get_heuristic_value(heuristic); // hvalue of successor
            node.open(parent_node, last_op);

            if (h < current_eval_context.get_heuristic_value(heuristic)) {	// if hvalue of successor < hvalue of parent
            	++num_ehc_phases;
                if (d_counts.count(d) == 0) {
                    d_counts[d] = make_pair(0, 0);
                }
                pair<int, int> &d_pair = d_counts[d];
                d_pair.first += 1;
                d_pair.second += statistics.get_expanded() - last_num_expanded;

                vector<const GlobalOperator*> ops;
                ops.push_back(last_op);
                used_actions[current_eval_context.get_state().get_id()] = ops;
                current_eval_context = eval_context;
                open_list->clear();
                return IN_PROGRESS;
            } else {
            	// Contribution to make plateaus RWs. do not expand child...
            	// ..only consume immediate neighbors and if no appropriate state is found, do RWs
                //expand(eval_context, d);
            }
        }
    }

    // open list is empty..perform RWs or return no solution after enough trials

    int current_hvalue = current_eval_context.get_heuristic_value(heuristic);
    EvaluationContext eval_context = current_eval_context;
    int hvalue = current_hvalue;
    cout << "to beat: " << hvalue << endl;
    vector<GlobalState> walk;

    const int restart_rate = restart_probability;
    vector<const GlobalOperator *> actions;
    do {
    	if (timer->is_expired()) {
    		// This will never happen, timer handled by system
    		cout << "doing RW and calling timeout" << endl;
    		return TIMEOUT;
    	}

    	//int random_value = rand() % 100;	// Map values to probabilities * 100, [0,99]
	double random_value = this->get_probability();
    	if (random_value < restart_rate) {
    		// Restart
			eval_context = current_eval_context;
			walk.clear();
			actions.clear();
    	}

		vector<const GlobalOperator *> ops = get_biased_successors(eval_context);
		if (ops.size() == 0) {
			cout << "Pruned all operators -- doing a pseduo-restart" << endl;
			eval_context = current_eval_context;
			walk.clear();
			actions.clear();
		}
		else {
			const GlobalOperator* random_op = ops.at(rand() % ops.size());
			GlobalState state = g_state_registry->get_successor_state(eval_context.get_state(), *random_op);

			//reach_state(eval_context.get_state(), *random_op, state);

			eval_context = EvaluationContext(state, &statistics);	// Eval Context of successor
			statistics.inc_evaluated_states();	// Evaluating random state
			statistics.inc_expanded();	// Expanding current state
			statistics.inc_generated();	// Only generating one (random) state
			// should inc_expanded_states() or inc_generated_states()?
			hvalue = eval_context.get_heuristic_value(heuristic); // hvalue of successor

			walk.push_back(state);
			actions.push_back(random_op);
		}
    } while (hvalue >= current_hvalue);

    used_actions[current_eval_context.get_state().get_id()] = actions;
    current_eval_context = eval_context;
    return IN_PROGRESS;
    //cout << "No solution - FAILED" << endl;
    //return FAILED;
}

long EnforcedHillClimbingSearchRWFixedProbability::luby_sequence(long sequence_number) {
	long focus = 2L;
	while (sequence_number > (focus - 1)) {
		focus = focus << 1;
	}

	if (sequence_number == (focus - 1)) {
		return focus >> 1;
	}
	else {
		return luby_sequence(sequence_number - (focus >> 1) + 1);
	}
}

void EnforcedHillClimbingSearchRWFixedProbability::print_statistics() const {
	for (int i = 0; i < num_execution_samples; ++i) {
		stats[i].print_detailed_statistics();
	}

	cout << "For last execution: " << endl;
    cout << "EHC phases: " << num_ehc_phases << endl;
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
}

static SearchEngine *_parse(OptionParser &parser) {
    parser.document_synopsis("Lazy enforced hill-climbing with RW in plateaus", "");
    parser.add_option<Heuristic *>("h", "heuristic");
    vector<string> preferred_usages;
    preferred_usages.push_back("DO_NOTHING");
    preferred_usages.push_back("PRUNE_BY_PREFERRED");
    preferred_usages.push_back("RANK_PREFERRED_FIRST");
    parser.add_enum_option(
        "preferred_usage",
        preferred_usages,
        "preferred operator usage",
        "DO_NOTHING");
    parser.add_list_option<Heuristic *>(
        "preferred",
        "use preferred operators of these heuristics",
        "[]");

    parser.add_option<double>("prob", "restart probability (0 <= r <= 1)");
    parser.add_option<double>("pref_prob", "probability of selecting a preferred operator in local minima (to create distributions. 0 means zero probability of selecting a preferred operator. -1 means do not add any bias (treat pref and unpref the same). Valid input is [0,1])", "-1");
    parser.add_option<int>("runs", "number of execution to sample randomness over", "1");

    SearchEngine::add_options_to_parser(parser);
    Options opts = parser.parse();

    if (parser.dry_run())
        return nullptr;
    else
        return new EnforcedHillClimbingSearchRWFixedProbability(opts);
}

static Plugin<SearchEngine> _plugin("ehc_rw_fixed_prob", _parse);
