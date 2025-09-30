#ifndef STRATEGY_TYPES_H
#define STRATEGY_TYPES_H

#include <cstdint>
#include <string>

// Common data structures
struct Candle {
    int64_t time;
    double open, high, low, close, volume;
};

struct RenkoBrick {
    int64_t brick_time, brick_start_time;
    double src_open, src_high, src_low, src_close;
    double open, high, low, close;
    int dir;
    bool reversal;
};

struct IchimokuRow {
    int64_t brick_time;
    double close;
    double tenkan, kijun, span_a, span_b, chikou;
};

struct Trade {
    int64_t entry_time;
    double entry_price;
    int64_t exit_time;
    double exit_price;
    std::string direction;
    double profit;
};

struct StrategyConfig {
    std::string symbol = "ETHUSDT";
    std::string resolution = "5m";
    double brick_size = 40.0;
    double reversal_size = 80.0;
    std::string source_type = "ohlc4";
    std::string start_date = "2025-08-01";
    std::string start_time = "00:00:00";
    std::string end_date = "2025-09-01";
    std::string end_time = "23:59:59";
    
    // Ichimoku parameters
    int tenkan = 5;
    int kijun = 26;
    int span_b = 52;
    int displacement = 26;
};

#endif 