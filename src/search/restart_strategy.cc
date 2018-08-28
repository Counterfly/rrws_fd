#include "restart_strategy.h"

RestartStrategy::RestartStrategy()
	: internal_sequence_count(1L) {}

RestartStrategy::RestartStrategy(long sequence_start_value)
	: internal_sequence_count(sequence_start_value)
{
	// Assert sequence_start_value > 0
}

RestartStrategy::~RestartStrategy()
{}

void RestartStrategy::reset_sequence()
{
	internal_sequence_count = 1L;
}
