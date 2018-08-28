#include "plateau_statistics.h"

#include <cassert>
#include "globals.h"
#include "timer.h"
#include "utilities.h"
#include "evaluation_context.h"

#include <iostream>
#include <vector>
using std::cout;
using std::endl;

PlateauStatistics::PlateauStatistics(int _depth_in_search_space) :
		depth_in_search_space(_depth_in_search_space), stats_per_depth() {
	stats_per_depth.push_back(new Statistics()); // Depth 0 is trivially 0 'escape' states over '1' total states (just ignore it)
}

PlateauStatistics::PlateauStatistics(const PlateauStatistics& other) {
	Statistics* stats;
	for (unsigned int i = 0; i < other.stats_per_depth.size(); ++i) {
		stats = new Statistics(*other.stats_per_depth.at(i));
		this->stats_per_depth.push_back(stats);
	}
}

PlateauStatistics::~PlateauStatistics() {
	for (unsigned int i = 0; i < this->stats_per_depth.size(); ++i) {
		delete this->stats_per_depth.at(i);
	}
}

PlateauStatistics PlateauStatistics::operator=(const PlateauStatistics& other) {
	return PlateauStatistics(other);
}

void PlateauStatistics::print_basic_statistics() const {
	//cout << get_evaluated_states() << " evaluated, " << get_expanded_states()
	//		<< " expanded, ";

	/*
	 if (reopened_states > 0) {
	 cout << reopened_states << " reopened, ";
	 }
	 cout << "t=" << g_timer;
	 cout << ", " << get_peak_memory_in_kb() << " KB";
	 */
}

void PlateauStatistics::print_detailed_statistics() const {

	Statistics* stats;
	for (unsigned int i = 0; i < this->stats_per_depth.size(); ++i) {
		stats = this->stats_per_depth.at(i);
		cout << "Stats at depth " << i << endl;
		cout << "\tNumber of states: " << stats->num_states;
		cout << ", " << stats->escape_states.size() << "escapes" << endl;
	}
	/*
	 if (lastjump_f_value >= 0) {
	 cout << "Expanded until last jump: "
	 << lastjump_expanded_states << " state(s)." << endl;
	 cout << "Reopened until last jump: "
	 << lastjump_reopened_states << " state(s)." << endl;
	 cout << "Evaluated until last jump: "
	 << lastjump_evaluated_states << " state(s)." << endl;
	 cout << "Generated until last jump: "
	 << lastjump_generated_states << " state(s)." << endl;
	 }
	 */
}

void PlateauStatistics::inc_num_states(int depth, int inc) {
	assert(depth - stats_per_depth.size() <= 1);
	if (stats_per_depth.size() <= (unsigned) depth)
		stats_per_depth.push_back(new Statistics());

	stats_per_depth[depth]->num_states += inc;
}

void PlateauStatistics::found_escape(EvaluationContext *context, int depth) {
	assert(stats_per_depth.size() - depth >= (size_t) 1); // Can only find solutions at depths we have seen before
	Statistics* stats = this->stats_per_depth.at(depth);
	stats->escape_states.push_back(context);
}

int PlateauStatistics::get_num_states() const {
	return stats_per_depth.back()->num_states;
}

int PlateauStatistics::get_num_escapes() const {
	Statistics* stats;
	int count = 0;
	for (unsigned int i = 1; i <= stats_per_depth.size(); ++i) {
		stats = stats_per_depth.at(i);
		count += stats->escape_states.size();
	}
	return count;
}

int PlateauStatistics::get_depth() const {
	return stats_per_depth.size() - 1; // @depth=0 is root of plateau with size==1
}

int PlateauStatistics::first_escape_depth() const {

	Statistics* stats;
	for (unsigned int i = 1; i <= this->stats_per_depth.size(); ++i) {
		stats = stats_per_depth.at(i);
		if (!stats->escape_states.empty()) {
			return i;
		}
	}
	return -1;
}

EvaluationContext* PlateauStatistics::get_escape_context() {
	// return context of first escape

	Statistics* stats;
	for (unsigned int i = 1; i <= this->stats_per_depth.size(); ++i) {
		stats = stats_per_depth.at(i);
		if (!stats->escape_states.empty()) {
			return stats->escape_states.front();
		}
	}
	return NULL;
}

void PlateauStatistics::print_statistics() const {
	Statistics* stats;
	cout << "Depth in Search Space" << "\t" << depth_in_search_space << endl;
	cout << "Number of Escapes" << "\t" << "Number of States" << endl;
	for (unsigned int i = 1; i < this->stats_per_depth.size(); ++i) {
		stats = stats_per_depth.at(i);
		cout << stats->escape_states.size() << "\t" << stats->num_states
				<< endl;
	}
	/*
	 cout << leader << "Number of escapes (could be -1): "
	 << this->get_num_escapes() << endl;
	 ;
	 leader = leader + "\t";
	 for (int i = 0; i < this->get_num_escapes(); ++i) {
	 Statistics stats = this->escape_snapshots.at(i);
	 cout << leader << "Escape " << (i + 1) << endl;
	 cout << leader << "\tGenerated States " << stats.generated_states
	 << endl;
	 cout << leader << "\tExpanded States " << stats.expanded_states << endl;
	 cout << leader << "\tEvaluated States " << stats.evaluated_states
	 << endl;
	 cout << leader << "\tDead Ends " << stats.dead_end_states << endl;
	 }
	 */
}
