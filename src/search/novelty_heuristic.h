#ifndef NOVELTY_HEURISTIC_H
#define NOVELTY_HEURISTIC_H

#include "heuristic.h"
#include "global_state.h"

#include <unordered_set>
#include <vector>

//typedef std::vector<std::vector<int>> IntRelation;

using std::string;
using std::vector;
using std::unordered_set;

class NoveltyHeuristic: public Heuristic {
	unsigned int max_novelty; // Calculate novelty values from 1 to max_novelty-1, states with worse novelty will be assigned max_novelty

	unordered_set<GlobalState, GlobalStateHash> explored_states; // GlobalStates that have been removed from the open list at least once

	/**
	 * Double vector to coincide with g_variable_names and g_variable_domains defined in globals.h
	 * Outer vector indexes the proposition ID, the inner vector indexes that proposition's domain,
	 * 	where a domain element being True means that assignment is in at least one state which is on the closed list.
	 *
	 * Think of this as a set of propositional assignments the search has 'seen' (those states are in the closed list).
	 * More Object-oriented programatically:
	 *   Set<Proposition> (where proposition object would have name:string, domain:vector)
	 *   Hash<Proposition, domain> (for faster lookup but requires longer setup)
	 */
	// vector<vector<bool>> prior_propositions;
	/*
	 * It would be nice if we could encapsulate the "used" (proposition, value)s in the heuristic,
	 *  but this would require the search engine to inform this class when a state is first put on the closed list.
	 *  -> seems weird for the search engine to tell a heuristic which node/state it selected to 'expand'.
	 *
	 */
	bool compute_novelty(const State &current_state, const int width);
	void generate_combinations(const int n, const int k,
			vector<vector<int>> &combinations) const;
protected:
	//virtual void initialize();
	virtual int compute_heuristic(const GlobalState &global_state);
	virtual int compute_heuristic(const State &state);
	/*
	 * This might work to use a local look-up table for storing propositions "seen"
	 virtual bool reach_state(
	 const GlobalState &parent_state, const GlobalOperator &op,
	 const GlobalState &state);
	 */
public:
	explicit NoveltyHeuristic(const Options &options);
	virtual ~NoveltyHeuristic()
override	;

	// Used to update the set of propositions the search has seen thus far
	virtual void reached_state(
			const GlobalState &parent_state, const GlobalOperator &op,
			const GlobalState &state);
};

#endif
