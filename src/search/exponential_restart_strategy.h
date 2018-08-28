#ifndef EXPONENTIAL_RESTART_STRATEGY_H
#define EXPONENTIAL_RESTART_STRATEGY_H

#include "restart_strategy.h"
#include <cstdint>

class ExponentialRestartStrategy : public RestartStrategy {
public:
	ExponentialRestartStrategy();
	ExponentialRestartStrategy(long sequence_start_value);
    ~ExponentialRestartStrategy();

public:
	virtual uint64_t next_sequence_value() override;
    virtual uint64_t sequence(long sequence_number) override;
};

#endif
