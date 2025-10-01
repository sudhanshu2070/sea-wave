#pragma once
#include <string>
#include <vector>
#include "types.hpp"

namespace backtest {

std::vector<Candle> fetch_candles(
    const std::string& symbol,
    const std::string& resolution,
    long long start_time,
    long long end_time,
    int limit = 4000
);

} // namespace backtest