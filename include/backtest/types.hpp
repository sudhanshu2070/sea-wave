#pragma once
#include <string>
#include <vector>

namespace backtest {

struct Candle {
    long long time;
    double open, high, low, close, volume;
};

struct RenkoBrick {
    long long brick_time;
    long long brick_start_time;
    double src_open, src_high, src_low, src_close;
    double open, high, low, close;
    int dir;
    bool reversal;
};

struct Trade {
    long long entry_time, exit_time;
    double entry_price, exit_price;
    std::string direction;
    double profit;
};

struct LogEntry {
    long long time;
    std::string action, reason;
    double close, kijun, cloud_top, cloud_bottom;
    bool long_entry_signal, long_exit_signal;
    bool short_entry_signal, short_exit_signal;
    std::string position;
    int available_renko;
};

} // namespace backtest