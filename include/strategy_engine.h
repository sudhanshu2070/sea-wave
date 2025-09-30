#ifndef STRATEGY_ENGINE_H
#define STRATEGY_ENGINE_H

#include "strategy_types.h"
#include <nlohmann/json.hpp>
#include <vector>

using json = nlohmann::json;

namespace StrategyEngine {
    json runBacktest(const StrategyConfig& config);
}

#endif 