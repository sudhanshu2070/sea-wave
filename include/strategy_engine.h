#ifndef STRATEGY_ENGINE_H
#define STRATEGY_ENGINE_H

#include "strategy_types.h"
#include <nlohmann/json.hpp>
#include <vector>
#include <string>

using json = nlohmann::json;

namespace StrategyEngine {
    struct BacktestResult {
        json summary;
        json config_used;
        json trades;
        json renko_data;
    };
    
    BacktestResult runBacktest(const StrategyConfig& config);
    std::string generateCSV(const BacktestResult& result, const std::string& type);
}

#endif 