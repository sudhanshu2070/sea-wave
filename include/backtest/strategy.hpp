#pragma once
#include <vector>
#include <tuple>
#include "types.hpp"

namespace backtest {

std::pair<std::vector<Trade>, std::vector<LogEntry>> run_strategy(
    const std::vector<RenkoBrick>& bricks,
    int tenkan_len, int kijun_len, int span_b_len
);

} // namespace backtest