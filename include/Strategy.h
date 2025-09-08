// Strategy.h
// Renko + Ichimoku Strategy
#ifndef STRATEGY_H
#define STRATEGY_H

#include <vector>
#include "OHLC.h"
#include "Renko.h"
#include "Ichimoku.h"
#include "Trade.h"
#include <fstream>

class Strategy {
private:
    double brick_size;
    int conv, base, spanb, disp;

public:
    Strategy(double bs = 40.0, int c = 5, int b = 26, int sb = 52, int d = 26)
        : brick_size(bs), conv(c), base(b), spanb(sb), disp(d) {}

    std::vector<Trade> backtest(const std::vector<OHLC>& ohlc_data) {
        RenkoBuilder renko(brick_size);
        renko.buildFromOHLC(ohlc_data);
        auto renko_bars = renko.getRenkoBars();
        renko.exportToCSV("renko_bricks.csv");

        Ichimoku ichimoku(conv, base, spanb, disp);
        auto lines = ichimoku.calculate(renko_bars);

        std::vector<Trade> trades;
        bool in_position = false;
        Trade current_trade;

        for (size_t i = std::max(base, spanb) + disp; i < renko_bars.size(); ++i) {
            double price = renko_bars[i].close;
            double tenkan = lines.tenkan_sen[i];
            double kijun = lines.kijun_sen[i];
            double span_a = lines.senkou_span_a[i];
            double span_b = lines.senkou_span_b[i];
            double chikou = lines.chikou_span[i - disp];  // Chikou is past close

            double cloud_top = std::max(span_a, span_b);
            double cloud_bottom = std::min(span_a, span_b);

            // Simple strategy: Buy when Tenkan > Kijun, price > cloud, chikou > price (bullish)
            // Sell when Tenkan < Kijun, price < cloud, chikou < price (bearish)
            // Exit on opposite signal

            bool buy_signal = (tenkan > kijun) && (price > cloud_top) && (chikou > renko_bars[i - disp].close);
            bool sell_signal = (tenkan < kijun) && (price < cloud_bottom) && (chikou < renko_bars[i - disp].close);

            if (!in_position) {
                if (buy_signal) {
                    current_trade.entry_time = renko_bars[i].time;
                    current_trade.entry_price = price;
                    current_trade.direction = "long";
                    in_position = true;
                } else if (sell_signal) {
                    current_trade.entry_time = renko_bars[i].time;
                    current_trade.entry_price = price;
                    current_trade.direction = "short";
                    in_position = true;
                }
            } else {
                bool exit_long = sell_signal || (tenkan < kijun);
                bool exit_short = buy_signal || (tenkan > kijun);

                if ((current_trade.direction == "long" && exit_long) ||
                    (current_trade.direction == "short" && exit_short)) {
                    current_trade.exit_time = renko_bars[i].time;
                    current_trade.exit_price = price;
                    if (current_trade.direction == "long") {
                        current_trade.profit = (current_trade.exit_price - current_trade.entry_price);
                    } else {
                        current_trade.profit = (current_trade.entry_price - current_trade.exit_price);
                    }
                    trades.push_back(current_trade);
                    in_position = false;
                }
            }
        }

        // Close open position at end
        if (in_position) {
            current_trade.exit_time = renko_bars.back().time;
            current_trade.exit_price = renko_bars.back().close;
            if (current_trade.direction == "long") {
                current_trade.profit = (current_trade.exit_price - current_trade.entry_price);
            } else {
                current_trade.profit = (current_trade.entry_price - current_trade.exit_price);
            }
            trades.push_back(current_trade);
        }

        exportTradesToCSV(trades, "trade_executions.csv");
        return trades;
    }

    void exportTradesToCSV(const std::vector<Trade>& trades, const std::string& filename) {
        std::ofstream file(filename);
        if (!file) return;
        file << "entry_time,entry_price,direction,exit_time,exit_price,profit\n";
        for (const auto& trade : trades) {
            file << trade.entry_time << "," << trade.entry_price << "," << trade.direction << ","
                 << trade.exit_time << "," << trade.exit_price << "," << trade.profit << "\n";
        }
        file.close();
    }
};

#endif