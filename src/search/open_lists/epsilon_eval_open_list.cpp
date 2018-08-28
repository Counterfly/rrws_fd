#include "epsilon_eval_open_list.h"

#include "../scalar_evaluator.h"

using namespace std;

EpsilonEvalOpenList::do_insertion(EvaluationContext &eval_context, const Entry &entry)
{
	primary_open_list->insert(eval_context, entry);
	secondary_open_list->insert(eval_context, entry);
}

template<class Entry>
Entry EpsilonEvalOpenList<Entry>::remove_min(vector<int> *key = 0)
{
	if (key)
	{
		cerr << "not implemented -- see msg639 in the tracker" << endl;
		exit_with(EXIT_UNSUPPORTED);
	}

	assert(primary_open_list->size() == secondary_open_list->size());
	assert(!primary_open_list->empty());

	OpenList<Entry> *focus_list = primary_open_list;
	//OpenList<Entry> *other_list = secondary_open_list;
	if (g_rng() < epsilon)
	{
		// Remove node based on secondary heuristic
		focus_list = secondary_open_list;
		//other_list = primary_open_list;
	}

	assert(!focus_list->empty());
	return focus_list->remove_min();
}

bool EpsilonEvalOpenList::is_dead_end(EvaluationContext &eval_context)
{
	return eval_context.is_heuristic_infinite(evaluator);
}

bool EpsilonEvalOpenList::is_reliable_dead_end(
		EvaluationContext &eval_context)
{
	return is_dead_end(eval_context) && evaluator->dead_ends_are_reliable();
}

void EpsilonEvalOpenList::get_involved_heuristics(std::set<Heuristic *> &hset)
{
	evaluator->get_involved_heuristics(hset);
}


bool EpsilonEvalOpenList::empty()
{
	return primary_open_list->empty() && secondary_open_list->empty();
}

void EpsilonEvalOpenList::clear()
{
	heap.clear();
	size = 0;
	next_id = 0;
}
