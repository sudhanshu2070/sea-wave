// Trade.h
// Simple struct for trades
#ifndef TRADE_H
#define TRADE_H

#include <string>

struct Trade {
    long entry_time;
    double entry_price;
    std::string direction;  // "long" or "short"
    long exit_time = 0;
    double exit_price = 0.0;
    double profit = 0.0;
};

#endif