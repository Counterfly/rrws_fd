#ifndef ENFORCED_HILL_CLIMBING_SEARCH_RW_FIXED_PROB_H
#define ENFORCED_HILL_CLIMBING_SEARCH_RW_FIXED_PROB_H

#include "evaluation_context.h"
#include "search_engine.h"
#include "search_statistics.h"
#include "countdown_timer.h"

#include "open_lists/open_list.h"
#include "ehc_rw.h"

#include <map>
#include <set>
#include <utility>
#include <vector>
#include "state_id.h"

class GEvaluator;
class Options;
class RestartStrategy;

typedef std::pair<StateID, std::pair<int, const GlobalOperator * >> OpenListEntryEHC;

/*
  Enforced hill-climbing with deferred evaluation.

  TODO: We should test if this lazy implementation really has any benefits over
  an eager one. We hypothesize that both versions need to evaluate and store
  the same states anyways.
*/
class EnforcedHillClimbingSearchRWFixedProbability : public SearchEngine {
    std::vector<const GlobalOperator *> get_successors(
        EvaluationContext &eval_context);
    std::vector<const GlobalOperator *> get_biased_successors(
               EvaluationContext &eval_context);	// Used only in RW section
    void expand(EvaluationContext &eval_context, int d);
    void reach_state(
        const GlobalState &parent, const GlobalOperator &op,
        const GlobalState &state);
    SearchStatus ehc();

    OpenList<OpenListEntryEHC> *open_list;
    GEvaluator *g_evaluator;

    Heuristic *heuristic;
    std::vector<Heuristic *> preferred_operator_heuristics;
    std::set<Heuristic *> heuristics;
    bool use_preferred;
    PreferredUsage preferred_usage;

    double restart_probability;
    double probability_preferred;		// Probability of selecting a preferred operator in successor generation (to create non-uniform distributions)
    int num_execution_samples;	// Number of executions to sample over randomness
    std::vector<SearchStatistics> stats;

    EvaluationContext current_eval_context;

    // Statistics
    std::map<int, std::pair<int, int>> d_counts;
    int num_ehc_phases;
    int last_num_expanded;

    // These two variables are for reconstructing the solution path
	std::map<StateID, std::vector<const GlobalOperator*>> used_actions;
	std::map<StateID, StateID> next_state;	// next state to go to after performing used_actions[StateID] in StateID

    CountdownTimer* timer;
protected:
    virtual void initialize() override;
    virtual SearchStatus step() override;

    // So that we can extract proper plan.
    //  ow, using the parent pointer won't work for RWs as a state may be revisited and create an inf loop
    virtual bool check_goal_and_set_plan(const GlobalState &state) override;

public:
    explicit EnforcedHillClimbingSearchRWFixedProbability(const Options &opts);
    virtual ~EnforcedHillClimbingSearchRWFixedProbability() override;

    virtual void print_statistics() const override;

    virtual void search() override;

    virtual void save_plan_if_necessary() const override;

private:
    long luby_sequence(long sequence_number);
};

#endif
