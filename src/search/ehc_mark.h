#ifndef EHC_H
#define EHC_H

#include "evaluation_context.h"
#include "search_engine.h"
#include "plateau_statistics.h"
#include "fetch_type.h"

#include "open_lists/open_list.h"

#include <map>
#include <set>
#include <utility>
#include <vector>

class ScalarEvaluator;
class Options;

// (EvalContext, depth in plateau (0 if not in plateau)), (g-value, operator taken to reach StateID)
//typedef std::pair<std::pair<StateID, int>,
//		std::pair<int, const GlobalOperator *>> OpenListEntryEHC;
class OpenListEntryEHC {

public:
	OpenListEntryEHC(StateID parent_state_id,
			EvaluationContext open_list_context, int depth, int g_value,
			const GlobalOperator* op) :
			parent_state_id(parent_state_id), open_list_context(
					open_list_context), depth(depth), g_value(g_value), op(op) {
	}
	StateID parent_state_id;
	EvaluationContext open_list_context;
	int depth, g_value;
	const GlobalOperator* op;
};

enum class PreferredUsage {
	PRUNE_BY_PREFERRED, RANK_PREFERRED_FIRST
};

/*
 Enforced hill-climbing with deferred evaluation.

 TODO: We should test if this lazy implementation really has any benefits over
 an eager one. We hypothesize that both versions need to evaluate and store
 the same states anyways.
 */
class EHC: public SearchEngine {
	friend std::ostream& operator<<(std::ostream&, const FetchType&);
	std::vector<const GlobalOperator *> get_successors(
			EvaluationContext &eval_context);
	void expand(EvaluationContext &eval_context, int g_relative, int depth);
	void reach_state(const GlobalState &parent, const GlobalOperator &op,
			const GlobalState &state);
	SearchStatus ehc();

	OpenList<OpenListEntryEHC> *open_list;
	ScalarEvaluator *evaluator;

	Heuristic *heuristic;
	std::vector<Heuristic *> preferred_operator_heuristics;
	std::set<Heuristic *> heuristics;
	bool use_preferred;
	PreferredUsage preferred_usage;

	EvaluationContext current_eval_context;

	// Statistics
	std::map<int, std::pair<int, int>> d_counts;
	int num_ehc_phases;
	int last_num_expanded;

	int num_plateau_phases;
	int num_non_plateau_phases;
	int num_plateau_recursions;
	int num_actions_taken; /* Essentially the depth in the search tree we are at */
	std::vector<PlateauStatistics *> plateau_statistics;
	FetchType* fetch;

	const int MIN_PLATEAU_DEPTH = 2;//5; // Depth of tree to be considered a plateau
	const int ESCAPE_PLATEAU_DEPTH_INC = 8; // Depth level to expand to past the first escape. (Will expand every depth up to (not including) this depth)

protected:
	virtual void initialize()
override	;
	virtual SearchStatus step() override;
	virtual void expand_plateau();
	virtual bool in_plateau() const;
	virtual bool stop_plateau_search(int) const;

public:
	explicit EHC(const Options &opts);
	virtual ~EHC() override;

	virtual void print_statistics() const override;
};

#endif
