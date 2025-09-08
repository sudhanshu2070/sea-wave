// Renko.h
// RenkoBuilder class
#ifndef RENKO_H
#define RENKO_H

#include <vector>
#include "OHLC.h"
#include <fstream>
#include <string>

class RenkoBuilder {
private:
    double brick_size;
    std::vector<OHLC> renko_bars;
    double last_price;
    bool initialized;

public:
    RenkoBuilder(double size = 40.0) : brick_size(size), initialized(false) {}

    void addPrice(double price, long timestamp) {
        if (!initialized) {
            last_price = price;
            initialized = true;
            return;
        }

        double diff = price - last_price;
        int bricks = static_cast<int>(std::abs(diff) / brick_size);
        if (bricks == 0) return;

        bool is_up = diff > 0;
        for (int i = 0; i < bricks; ++i) {
            OHLC bar;
            bar.time = timestamp;  // Approximate, can improve
            if (is_up) {
                bar.open = last_price;
                bar.close = last_price + brick_size;
                bar.high = bar.close;
                bar.low = bar.open;
            } else {
                bar.open = last_price;
                bar.close = last_price - brick_size;
                bar.high = bar.open;
                bar.low = bar.close;
            }
            renko_bars.push_back(bar);
            last_price = bar.close;
        }
    }

    void buildFromOHLC(const std::vector<OHLC>& ohlc_data) {
        for (const auto& bar : ohlc_data) {
            // For simplicity, use close. For accuracy, check high/low for intra-bar bricks.
            addPrice(bar.close, bar.time);
        }
    }

    const std::vector<OHLC>& getRenkoBars() const {
        return renko_bars;
    }

    void exportToCSV(const std::string& filename) const {
        std::ofstream file(filename);
        if (!file) return;
        file << "time,open,high,low,close\n";
        for (const auto& bar : renko_bars) {
            file << bar.time << "," << bar.open << "," << bar.high << "," << bar.low << "," << bar.close << "\n";
        }
        file.close();
    }
};

#endif