#ifndef PLATEAU_STATISTICS_H
#define PLATEAU_STATISTICS_H

#include "evaluation_context.h"

#include <vector>
/*
 This class keeps track of search statistics.

 It keeps counters for expanded, generated and evaluated states (and
 some other statistics) and provides uniform output for all search
 methods.
 */

class Statistics {
public:
	Statistics() :
			num_states(0), escape_states() {
	}

	Statistics(const Statistics &other) :
			num_states(other.num_states) {
		for (unsigned int i = 0; i < other.escape_states.size(); ++i) {
			escape_states.push_back(other.escape_states.at(i));
		}
	}
	// General statistics
	int num_states;
	std::vector<EvaluationContext*> escape_states; // context of states with hvalue less than plateau root
};

class PlateauStatistics {
	int depth_in_search_space;
	/*
	 * vector(index) represents the statistics at depth=index
	 */
	std::vector<Statistics*> stats_per_depth; // Statistics with scope between each escape that is found in plateau
	void print_f_line() const;
public:
	PlateauStatistics(int);
	PlateauStatistics(const PlateauStatistics&); // copy constructor
	~PlateauStatistics();

	PlateauStatistics operator=(const PlateauStatistics&);

	// Methods that update statistics.
	void inc_num_states(int depth, int = 1);
	void found_escape(EvaluationContext*, int);

	// Methods that access statistics.
	int get_num_states() const;
	int get_num_escapes() const;
	int get_depth() const;
	int first_escape_depth() const;

	// Returns first escape context found in plateau
	EvaluationContext* get_escape_context();

	/*
	 Call the following method with the f value of every expanded
	 state. It will notice "jumps" (i.e., when the expanded f value
	 is the highest f value encountered so far), print some
	 statistics on jumps, and keep track of expansions etc. up to the
	 last jump.

	 Statistics until the final jump are often useful to report in
	 A*-style searches because they are not affected by tie-breaking
	 as the overall statistics. (With a non-random, admissible and
	 consistent heuristic, the number of expanded, evaluated and
	 generated states until the final jump is fully determined by the
	 state space and heuristic, independently of things like the
	 order in which successors are generated or the tie-breaking
	 performed by the open list.)
	 */
	void report_f_value_progress(int f);

	// output
	void print_basic_statistics() const;
	void print_detailed_statistics() const;
	void print_statistics() const;
};

#endif
