#ifndef LUBY_RESTART_STRATEGY_H
#define LUBY_RESTART_STRATEGY_H

#include "restart_strategy.h"
#include <cstdint>

class LubyRestartStrategy : public RestartStrategy {
public:
	LubyRestartStrategy();
	LubyRestartStrategy(long sequence_start_value);
    ~LubyRestartStrategy();

public:
	virtual uint64_t next_sequence_value() override;
    virtual uint64_t sequence(long sequence_number) override;
};

#endif
