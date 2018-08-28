#include "search_statistics.h"

#include "globals.h"
#include "timer.h"
#include "utilities.h"

#include "global_operator.h"

#include <iostream>
using namespace std;

SearchStatistics::SearchStatistics() :
	expanded_states(0),
	reopened_states(0),
	evaluated_states(0),
	evaluations(0),
	generated_states(0),
	dead_end_states(0),
	generated_ops(0),
	lastjump_f_value(-1),
	lastjump_expanded_states(0),
	lastjump_reopened_states(0),
	lastjump_evaluated_states(0),
	lastjump_generated_states(0),
	found_solution(false),
	plan()
{}

SearchStatistics::SearchStatistics(const SearchStatistics& other) :
		expanded_states(other.expanded_states), reopened_states(other.reopened_states),
				evaluated_states(other.evaluated_states), evaluations(other.evaluations), generated_states(
				other.generated_states), dead_end_states(
				other.dead_end_states), generated_ops(other.generated_ops), lastjump_f_value(
				other.lastjump_f_value), lastjump_expanded_states(
				other.lastjump_expanded_states), lastjump_reopened_states(
				other.lastjump_reopened_states), lastjump_evaluated_states(
				other.lastjump_evaluated_states), lastjump_generated_states(
				other.lastjump_generated_states),
				found_solution(other.found_solution), plan(other.plan) {

}

SearchStatistics& SearchStatistics::operator=(const SearchStatistics& other) {
	if (this != &other) {
		expanded_states = other.expanded_states;
		evaluated_states = other.evaluated_states;
		evaluations = other.evaluations;
		generated_states = other.generated_states;
		reopened_states = other.reopened_states;
		dead_end_states = other.dead_end_states;
		generated_ops = other.generated_ops;
		lastjump_f_value = other.lastjump_f_value;
		lastjump_expanded_states = other.lastjump_expanded_states;
		lastjump_reopened_states = other.lastjump_reopened_states;
		lastjump_evaluated_states = other.lastjump_evaluated_states;
		lastjump_generated_states = other.lastjump_generated_states;

		found_solution = other.found_solution;
		for (unsigned int i = 0; i < other.plan.size(); ++i) {
			plan.push_back(other.plan[i]);
		}
	}
	return *this;
}

void SearchStatistics::report_f_value_progress(int f) {
	if (f > lastjump_f_value) {
		lastjump_f_value = f;
		print_f_line();
		lastjump_expanded_states = expanded_states;
		lastjump_reopened_states = reopened_states;
		lastjump_evaluated_states = evaluated_states;
		lastjump_generated_states = generated_states;
	}
}

void SearchStatistics::print_f_line() const {
	cout << "f = " << lastjump_f_value << " [";
	print_basic_statistics();
	cout << "]" << endl;
}

void SearchStatistics::print_basic_statistics() const {
	cout << evaluated_states << " evaluated, " << expanded_states
			<< " expanded, ";
	if (reopened_states > 0) {
		cout << reopened_states << " reopened, ";
	}
	cout << "t=" << g_timer;
	cout << ", " << get_peak_memory_in_kb() << " KB";
}

void SearchStatistics::print_detailed_statistics() const {
	if (get_found_solution()) {
		unsigned int cost = 0;
		for (unsigned int i = 0; i < plan.size(); ++i) {
			cost += plan[i]->get_cost();
		}
		cout << "Plan Length: " << plan.size() << endl;
		cout << "Plan Cost: " << cost << endl;
	}
	else
	{
		cout << "No Solution Found." << endl;
	}
	cout << "Expanded " << expanded_states << " state(s)." << endl;
	cout << "Reopened " << reopened_states << " state(s)." << endl;
	cout << "Evaluated " << evaluated_states << " state(s)." << endl;
	cout << "Evaluations: " << evaluations << endl;
	cout << "Generated " << generated_states << " state(s)." << endl;
	cout << "Dead ends: " << dead_end_states << " state(s)." << endl;

	if (lastjump_f_value >= 0) {
		cout << "Expanded until last jump: " << lastjump_expanded_states
				<< " state(s)." << endl;
		cout << "Reopened until last jump: " << lastjump_reopened_states
				<< " state(s)." << endl;
		cout << "Evaluated until last jump: " << lastjump_evaluated_states
				<< " state(s)." << endl;
		cout << "Generated until last jump: " << lastjump_generated_states
				<< " state(s)." << endl;
	}
}
