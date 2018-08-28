#include "novelty_heuristic.h"

#include "plugin.h"
#include "globals.h"

#include <algorithm>
#include <vector>
#include <unordered_set>

NoveltyHeuristic::NoveltyHeuristic(const Options &options) :
		Heuristic(options), max_novelty(options.get<int>("max_novelty")) {
}

NoveltyHeuristic::~NoveltyHeuristic() {
}

void NoveltyHeuristic::reached_state(const GlobalState& /*parent_state*/,
		const GlobalOperator& /*op*/, const GlobalState& state) {
	explored_states.insert(state);
}

int NoveltyHeuristic::compute_heuristic(const GlobalState &global_state) {
	State state = convert_global_state(global_state);
	int n = compute_heuristic(state);
//	cout << "NoveltyHeuristic is " << n << std::endl;
	return n;
}

int NoveltyHeuristic::compute_heuristic(const State &state) {
	/*
	 * Quicker method to compute novelty=1
	 * Look at effect list and see if any of those propositions are different from history
	 * Don't see an immediate way to compute the prior effect list (would have to be the effect list of when this State/SearchNode was added to the openlist
	 */

	unsigned int MAX_WIDTH = max_novelty;
	// Computing Novelties > 1
	bool novel = false;
	for (unsigned int width = 1; width < MAX_WIDTH; ++width) {
		novel = compute_novelty(state, width);
		if (novel)
			return width;
	}

	return MAX_WIDTH; // State is not between 1..MAX_WIDTH-1 novel
}

bool NoveltyHeuristic::compute_novelty(const State &current_state,
		const int width) {
	if (explored_states.empty()) {
		return true;
	}
	//cout << "NoveltyHeuristic START past_States.size = " <<  explored_states.size() << std::endl;
	vector<int> &global_propositions = g_variable_domain; // PropID -> DomainSize
	int NUM_PROPOSITIONS = global_propositions.size();
	vector<vector<int>> combinations;
	vector<int> combination(width); // Combination of propositions
	bool state_novel; // Novel wrt specific state
	generate_combinations(NUM_PROPOSITIONS, width, combinations);
	for (unsigned int i = 0; i < combinations.size(); ++i) // Iterate over all possible proposition combinations of size width
			{
		combination = combinations[i];

		//cout << "NoveltyHeuristic combination[" << i << "] (width=" << width << ") = " << combination << std::endl;
		//combination.size() == width
		state_novel = true;
		//for (std::unordered_set<GlobalState>::iterator past_gstate = explored_states.begin(); past_gstate != explored_states.end() && !state_novel; ++past_gstate)
		for (auto past_gstate = explored_states.begin();
				past_gstate != explored_states.end() && state_novel;
				++past_gstate) {
			State past_state = convert_global_state(*past_gstate);
			state_novel = false;
			for (int j = 0; j < width && !state_novel; ++j) // Iterate over propositions in specific combination
					{
				int proposition_index = combination[j];

				//cout << "NoveltyHeuristic " << past_state[proposition_index].get_name() << "=" << past_state[proposition_index].get_value() <<
				//		"=?= " <<
				//		current_state[proposition_index].get_name() << "=" << current_state[proposition_index].get_value() << std::endl;

				// State[index] returns FactProxy
				// Current state is 'width' novel wrt to 'past_state' iff at least one of the propositions (in combination) value are not equal between the two states
				if (past_state[proposition_index]
						!= current_state[proposition_index]) {
					// 'past_state' does not have the same proposition value
					// thus, this combination of propositions would be novel relative to past_state
					state_novel = true; // Dont need to check other propositions, know state is 'width' novel wrt past_state
				}
			}

			// If state_novel == false, then immediately try new combination
		}

		if (state_novel)
			return true; // If state is 'width' novel wrt all past_states then return true
	}
	return false;
}

void NoveltyHeuristic::generate_combinations(const int n, const int k,
		vector<vector<int>> &combinations) const {
	std::string bitmask(k, 1); // k leading 1's
	bitmask.resize(n, 0); // n-k trailing 0's
	std::vector<int> combination;

	do {
		combination.clear();
		for (int i = 0; i < n; ++i) // [0..N-1] integers
				{
			if (bitmask[i])
				combination.push_back(i);
		}
		combinations.push_back(combination);
	} while (std::prev_permutation(bitmask.begin(), bitmask.end()));
}

static Heuristic *_parse(OptionParser &parser) {
	// Don't show documentation for this temporary class (see issue198).
	parser.document_hide();
	parser.document_synopsis("Novelty (blind) Heuristic", "");
	std::string DEFAULT_MAX_WIDTH = "2";
	parser.add_option<int>("max_novelty", "number of equivalence classes",
			DEFAULT_MAX_WIDTH, Bounds("1", "infinity"));
	Heuristic::add_options_to_parser(parser);
	Options opts = parser.parse();

	parser.document_property("admissible", "no");
	parser.document_property("consistent", "no");

	if (parser.dry_run())
		return 0;
	else
		return new NoveltyHeuristic(opts);
}

static Plugin<Heuristic> _plugin("novelty", _parse);
