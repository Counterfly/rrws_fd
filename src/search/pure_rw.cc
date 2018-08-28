#include "pure_rw.h"

#include "g_evaluator.h"
#include "global_operator.h"
#include "plugin.h"
#include "pref_evaluator.h"
#include "successor_generator.h"
#include "utilities.h"

#include <algorithm>  // for random_shuffle
#include <cstdint>
#include "stdio.h"	// for NULL
#include "stdlib.h" // for rand() and srand
#include "time.h"	// for time
#include "const_evaluator_mark.h"

#include <sys/time.h>
#define UNUSED(expr) do { (void)(expr); } while (0)
using namespace std;

PureRW::PureRW(
    const Options &opts)
    : SearchEngine(opts),
      probability_preferred(opts.get<int>("pref_prob")),
      num_execution_samples(opts.get<int>("runs")),
	  stats(num_execution_samples),
      current_eval_context(g_initial_state(), &statistics),
      plan(),
      last_num_expanded(-1) {
	// else prefusage == DO_NOTHING
    struct timeval time;
	gettimeofday(&time,NULL);

	// microsecond has 1 000 000
	// Assuming you did not need quite that accuracy
	// Also do not assume the system clock has that accuracy.
	srand((time.tv_sec * 1000) + (time.tv_usec / 1000));
}

PureRW::~PureRW() {
}

void PureRW::reach_state(
    const GlobalState &parent, const GlobalOperator &op, const GlobalState &state) {
	UNUSED(parent);
	UNUSED(op);
	UNUSED(state);
}

void PureRW::save_plan_if_necessary() const {
	bool multiple = num_execution_samples > 1;
	for (int i = 0; i < num_execution_samples; ++i) {
		if (stats[i].get_found_solution()) {
			save_plan(stats[i].get_plan(), multiple);
		}
	}
}

void PureRW::initialize() {
    cout << "Conducting enforced hill-climbing search, (real) bound = "
         << bound << endl;

    // Re-initialize
    clear_plan();
    d_counts.clear();

    SearchNode node = search_space.get_node(current_eval_context.get_state());
    node.open_initial();
    cout << "finitial initialize, current_Eval_context.id = " << current_eval_context.get_state().get_id() << endl;
}

vector<const GlobalOperator *> PureRW::get_successors(
    EvaluationContext &eval_context) {
    vector<const GlobalOperator *> ops;
	g_successor_generator->generate_applicable_ops(eval_context.get_state(), ops);
    statistics.inc_expanded();
    statistics.inc_generated_ops(ops.size());
    // Randomize ops
    std::random_shuffle(ops.begin(), ops.end());
    return ops;
}


vector<const GlobalOperator *> PureRW::get_biased_successors(
    EvaluationContext &eval_context) {
    vector<const GlobalOperator *> ops;
	g_successor_generator->generate_applicable_ops(eval_context.get_state(), ops);

    statistics.inc_expanded();
    statistics.inc_generated_ops(ops.size());

    // Randomize ops
    std::random_shuffle(ops.begin(), ops.end());
    return ops;
}

void PureRW::expand(EvaluationContext &eval_context, int d) {
	UNUSED(eval_context);
	UNUSED(d);
}

bool PureRW::check_goal_and_set_plan(const GlobalState &state) {
    if (test_goal(state)) {
        cout << "Solution found!" << endl;
        set_plan(plan);
        return true;
    }
    return false;
}

void PureRW::search() {
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


SearchStatus PureRW::step() {
    last_num_expanded = statistics.get_expanded();
    search_progress.check_progress(current_eval_context);

    if (check_goal_and_set_plan(current_eval_context.get_state())) {
    	cout << "step, returning SOLVED" << endl;
        return SOLVED;
    }

    //expand(current_eval_context, 0);
    return ehc();
}

SearchStatus PureRW::ehc() {
	// NOT ACTUALLY EHC, just a pure RW
    // open list is empty..perform RWs or return no solution after enough trials
    EvaluationContext eval_context = current_eval_context;

    eval_context = current_eval_context;
	while (!check_goal_and_set_plan(eval_context.get_state()))
	{
		if (timer->is_expired()) {
			// This will never happen, timer handled by system
			cout << "doing RW and calling timeout" << endl;
			return TIMEOUT;
		}
		//cout << eval_context.get_state().get_id() << " -> " << endl;
		vector<const GlobalOperator *> ops = get_biased_successors(eval_context);
		if (ops.size() == 0) {
			cout << "Pruned all operators -- exiting and marking as unsolvable" << endl;
			exit_with(EXIT_UNSOLVED_INCOMPLETE);
			//eval_context = current_eval_context;
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

    //for (std::vector<const GlobalState*>::iterator it = walk.begin(); it != walk.end(); ++it)
    //GlobalState parent_state = current_eval_context.get_state();
    //for (unsigned int i = 0; i < walk.size(); ++i)
    //for (GlobalState state : walk)
    //{
    	//GlobalState state = walk.at(i);
    	//cout << state.get_id() << "-->";
    	//const GlobalOperator* random_op = actions.at(i);
    	//const GobalState* state = *it;
    	//SearchNode search_node = search_space.get_node(state);
    	//search_node.update_parent(search_space.get_node(parent_state), random_op);
    	//parent_state = state;
    //}

    current_eval_context = eval_context;
    return SOLVED;
    //cout << "No solution - FAILED" << endl;
    //return FAILED;
}

long PureRW::luby_sequence(long sequence_number) {
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

void PureRW::print_statistics() const {
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

    parser.add_option<int>("pref_prob", "currently, not used. probability of selecting a preferred operator in local minima (to create distributions. 0 means zero probability of selecting a preferred operator. -1 means do not add any bias (treat pref and unpref the same). Valid input is [0,100])", "-1");
    parser.add_option<int>("runs", "number of execution to sample randomness over", "1");

    SearchEngine::add_options_to_parser(parser);
    Options opts = parser.parse();

    if (parser.dry_run())
        return nullptr;
    else
        return new PureRW(opts);
}

static Plugin<SearchEngine> _plugin("pure_rw", _parse);
