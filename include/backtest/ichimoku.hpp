#pragma once
#include <vector>

namespace backtest {

std::vector<double> donchian_mid(const std::vector<double>& series, int length);

} // namespace backtest