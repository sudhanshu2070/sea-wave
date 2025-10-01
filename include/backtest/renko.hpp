#pragma once
#include <vector>
#include "types.hpp"

namespace backtest {

std::vector<RenkoBrick> build_renko(
    const std::vector<Candle>& candles,
    double brick_size,
    double reversal_size
);

} // namespace backtest