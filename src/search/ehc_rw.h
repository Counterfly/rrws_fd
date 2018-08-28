#ifndef ENFORCED_HILL_CLIMBING_SEARCH_RW_H
#define ENFORCED_HILL_CLIMBING_SEARCH_RW_H

#include "evaluation_context.h"
#include "search_engine.h"
#include "search_statistics.h"
#include "countdown_timer.h"

#include "open_lists/open_list.h"

#include <map>
#include <set>
#include <utility>
#include <vector>
#include "state_id.h"

class GEvaluator;
class Options;
class RestartStrategy;

typedef std::pair<StateID, std::pair<int, const GlobalOperator * >> OpenListEntryEHC;

enum class PreferredUsage {
	DO_NOTHING,	/* using preferred operators but don't want to influence hill-climbing (maybe using in localminima*/
    PRUNE_BY_PREFERRED,
    RANK_PREFERRED_FIRST
};
inline std::ostream& operator<<(std::ostream& out, const PreferredUsage value){
    const char* s = 0;
    if (value == PreferredUsage::DO_NOTHING) {
    	s = "DO NOTHING";
    }
    else if (value == PreferredUsage::PRUNE_BY_PREFERRED) {
    	s = "PRUNE_BY_PREFERRED";
    }
    else if (value == PreferredUsage::RANK_PREFERRED_FIRST) {
    	s = "RANK_BY_PREFERRED_FIRST";
    }
    else {
    	s = "DNE, something went wrong";
    }

    return out << s;
}

/*
  Enforced hill-climbing with deferred evaluation.

  TODO: We should test if this lazy implementation really has any benefits over
  an eager one. We hypothesize that both versions need to evaluate and store
  the same states anyways.
*/
class EnforcedHillClimbingSearchRW : public SearchEngine {
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

    RestartStrategy* restart_strategy;
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
    //typedef StateIDInt StateID;
    std::map<StateID, StateID> next_state;	// next state to go to after performing used_actions[StateID] in StateID

    CountdownTimer* timer;
protected:
    virtual void initialize() override;
    virtual SearchStatus step() override;

    // So that we can extract proper plan.
    //  ow, using the parent pointer won't work for RWs as a state may be revisited and create an inf loop
    virtual bool check_goal_and_set_plan(const GlobalState &state) override;

public:
    explicit EnforcedHillClimbingSearchRW(const Options &opts);
    virtual ~EnforcedHillClimbingSearchRW() override;

    virtual void print_statistics() const override;

    virtual void search() override;

    virtual void save_plan_if_necessary() const override;

private:
    long luby_sequence(long sequence_number);
};

#endif
