#include "pure_rrw_fixed_probability.h"

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
#include "const_evaluator_mark.h"

#include <sys/time.h>
#define UNUSED(expr) do { (void)(expr); } while (0)
using namespace std;

PureRRWFixedProb::PureRRWFixedProb(
    const Options &opts)
    : SearchEngine(opts),
      scaling_heuristic(opts.get<Heuristic*>("scaling_heuristic")),
      scaling_factor(0),
      preferred_operator_heuristics(opts.get_list<Heuristic *>("preferred")),
      prob(opts.get<double>("prob")),
      probability_preferred(opts.get<double>("pref_prob")),
      num_execution_samples(opts.get<int>("runs")),
	  stats(num_execution_samples),
      current_eval_context(g_initial_state(), &statistics),
      plan(),
      last_num_expanded(-1) {

	use_preferred = preferred_operator_heuristics.size() > 0;


	// else prefusage == DO_NOTHING
        struct timeval time;
	gettimeofday(&time,NULL);

	// microsecond has 1 000 000
	// Assuming you did not need quite that accuracy
	// Also do not assume the system clock has that accuracy.
	srand((time.tv_sec * 1000) + (time.tv_usec / 1000));

        cout << "---------" << endl;
	cout << "Prob (as double) = " << prob << endl;
	cout << "Pref_Prob (as double) = " << probability_preferred << endl;
	exit_with(EXIT_UNSOLVED_INCOMPLETE);
}

PureRRWFixedProb::~PureRRWFixedProb() {
}

void PureRRWFixedProb::reach_state(
    const GlobalState &parent, const GlobalOperator &op, const GlobalState &state) {
	UNUSED(parent);
	UNUSED(op);
	UNUSED(state);
}

void PureRRWFixedProb::save_plan_if_necessary() const {
	bool multiple = num_execution_samples > 1;
	for (int i = 0; i < num_execution_samples; ++i) {
		if (stats[i].get_found_solution()) {
			save_plan(stats[i].get_plan(), multiple);
		}
	}
}

void PureRRWFixedProb::initialize() {
    cout << "Conducting enforced hill-climbing search, (real) bound = "
         << bound << endl;

    // Re-initialize
    clear_plan();
    d_counts.clear();

    SearchNode node = search_space.get_node(current_eval_context.get_state());
    node.open_initial();

    this->scaling_factor = current_eval_context.get_heuristic_value(this->scaling_heuristic);
    assert(this->scaling_factor > 0);
}

vector<const GlobalOperator *> PureRRWFixedProb::get_successors(
    EvaluationContext &eval_context) {
    vector<const GlobalOperator *> ops;
	g_successor_generator->generate_applicable_ops(eval_context.get_state(), ops);
    statistics.inc_expanded();
    statistics.inc_generated_ops(ops.size());
    // Randomize ops
    std::random_shuffle(ops.begin(), ops.end());
    return ops;
}


vector<const GlobalOperator *> PureRRWFixedProb::get_biased_successors(
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

void PureRRWFixedProb::expand(EvaluationContext &eval_context, int d) {
	UNUSED(eval_context);
	UNUSED(d);
}

bool PureRRWFixedProb::check_goal_and_set_plan(const GlobalState &state) {
    if (test_goal(state)) {
        cout << "Solution found!" << endl;
        set_plan(plan);
        return true;
    }
    return false;
}

void PureRRWFixedProb::search() {
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


SearchStatus PureRRWFixedProb::step() {
    last_num_expanded = statistics.get_expanded();
    search_progress.check_progress(current_eval_context);

    if (check_goal_and_set_plan(current_eval_context.get_state())) {
        return SOLVED;
    }

    //expand(current_eval_context, 0);
    return ehc();
}

SearchStatus PureRRWFixedProb::ehc() {
	// NOT ACTUALLY EHC, just a pure RW
    // open list is empty..perform RWs or return no solution after enough trials
    EvaluationContext eval_context = current_eval_context;

	while (!check_goal_and_set_plan(eval_context.get_state()))
	{
                if (plan.size() >= this->scaling_factor) {
                    /*
                     * The behaviour for scaling is to go to depth 'scaling_factor' with probability 1, then for each
                     * expansion thereafter, restart with probability 'this->prob'
                     */
		    double random_value = this->get_probability();	// Map values to probabilities
		    if (random_value < this->prob) {
		    	// Restart!
                        //cout << "restarting with length " << plan.size() << " and factor = " << this->scaling_factor << endl;
		    	eval_context = current_eval_context;
		    	plan.clear();
		    }
                }

		if (timer->is_expired()) {
			// This will never happen, timer handled by system
			cout << "doing RW and calling timeout" << endl;
			return TIMEOUT;
		}
		//cout << eval_context.get_state().get_id() << " -> " << endl;
		vector<const GlobalOperator *> ops = get_biased_successors(eval_context);
		if (ops.size() == 0) {
			cout << "Pruned all operators -- exiting and marking as unsolvable" << endl;
			//exit_with(EXIT_UNSOLVED_INCOMPLETE);
			eval_context = current_eval_context;
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

			plan.push_back(random_op);
		}
	}

    current_eval_context = eval_context;
    return SOLVED;
    //cout << "No solution - FAILED" << endl;
    //return FAILED;
}

long PureRRWFixedProb::luby_sequence(long sequence_number) {
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

void PureRRWFixedProb::print_statistics() const {
	for (int i = 0; i < num_execution_samples; ++i) {
		stats[i].print_detailed_statistics();
	}

	cout << "For last execution: " << endl;
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
    parser.document_synopsis("Single RW, no restarts, uniform probability", "");

    parser.add_option<Heuristic *>(
            "scaling_heuristic",
            "How to scale each RW before using the specific probability",
            "const(1)");
    parser.add_list_option<Heuristic *>(
            "preferred",
            "use preferred operators of these heuristics",
            "[]");
    parser.add_option<double>("prob", "fixed probability rate [0,1]", "0");
    parser.add_option<double>("pref_prob", "currently, not used. probability of selecting a preferred operator in local minima (to create distributions. 0 means zero probability of selecting a preferred operator. -1 means do not add any bias (treat pref and unpref the same). Valid input is [0,1])", "-1");
    parser.add_option<int>("runs", "number of execution to sample randomness over", "1");

    SearchEngine::add_options_to_parser(parser);
    Options opts = parser.parse();

    if (parser.dry_run())
        return nullptr;
    else
        return new PureRRWFixedProb(opts);
}

static Plugin<SearchEngine> _plugin("pure_rrw_fixed_prob", _parse);
