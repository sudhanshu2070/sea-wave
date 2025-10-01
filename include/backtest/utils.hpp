#pragma once
#include <string>
#include <vector>
#include "types.hpp"

namespace backtest {

long long ist_to_unix(const std::string& date_str, const std::string& time_str = "00:00:00");

std::string renko_to_csv(
    const std::vector<RenkoBrick>& bricks,
    const std::vector<double>& tenkan,
    const std::vector<double>& kijun,
    const std::vector<double>& span_a,
    const std::vector<double>& span_b
);

std::string trades_to_csv(const std::vector<Trade>& trades);
std::string logs_to_csv(const std::vector<LogEntry>& logs);

} // namespace backtest