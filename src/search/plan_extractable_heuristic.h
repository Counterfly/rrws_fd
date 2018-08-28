#ifndef PLAN_EXTRACTABLE_HEURISTIC_H
#define PLAN_EXTRACTABLE_HEURISTIC_H

#include "heuristic.h"
#include "search_engine.h"	// For Plan typedef


#include <string>
typedef SearchEngine::Plan Plan;

using std::string;
class PlanExtractableHeuristic: public Heuristic {

protected:
	virtual int compute_heuristic(const GlobalState &state) = 0;
public:
	PlanExtractableHeuristic(const Options&);
	virtual ~PlanExtractableHeuristic();
	virtual Plan extract_plan() = 0;
};

#endif
