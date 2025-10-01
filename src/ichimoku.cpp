#include "../include/backtest/ichimoku.hpp"
#include <algorithm>
#include <cmath>

namespace backtest {

std::vector<double> donchian_mid(const std::vector<double>& series, int length) {
    std::vector<double> result(series.size(), NAN);
    if (length <= 0 || static_cast<size_t>(length) > series.size()) {
        return result;
    }
    for (size_t i = length - 1; i < series.size(); ++i) {
        double hh = *std::max_element(series.begin() + i - length + 1, series.begin() + i + 1);
        double ll = *std::min_element(series.begin() + i - length + 1, series.begin() + i + 1);
        result[i] = (hh + ll) / 2.0;
    }
    return result;
}

} // namespace backtest