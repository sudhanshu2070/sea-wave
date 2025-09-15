#ifndef STRATEGY_H
#define STRATEGY_H

#include <vector>
#include <fstream>
#include <iomanip>
#include <ctime>
#include <iostream>
#include "OHLC.h"
#include "Renko.h"
#include "Ichimoku.h"
#include "Trade.h"

struct StrategyLog {
    long time;
    std::string action;
    std::string reason;
    double close, kijun, cloud_top, cloud_bottom;
    bool long_entry_signal, long_exit_signal, short_entry_signal, short_exit_signal;
    std::string position;
    int available_renko;
};

class Strategy {
private:
    double brick_size, reversal_size;
    int tenkan_len, kijun_len, span_b_len, displacement;

    std::string unixToIST(long unix_time) {
        unix_time += 19800;
        std::time_t t = static_cast<std::time_t>(unix_time);
        std::tm* tm = std::localtime(&t);
        std::ostringstream oss;
        oss << std::put_time(tm, "%Y-%m-%d %H:%M:%S") << " IST";
        return oss.str();
    }

public:
    Strategy(double bs = 40.0, double rs = 80.0, int t = 5, int k = 26, int sb = 52, int d = 26)
        : brick_size(bs), reversal_size(rs), tenkan_len(t), kijun_len(k), span_b_len(sb), displacement(d) {}

    std::vector<Trade> backtest(const std::vector<OHLC>& ohlc_data) {
        RenkoBuilder renko(brick_size, reversal_size);
        renko.buildFromOHLC(ohlc_data);
        auto renko_bars = renko.getRenkoBars();
        renko.exportToCSV("renko_with_ichimoku.csv");

        Ichimoku ichimoku(tenkan_len, kijun_len, span_b_len, displacement);
        auto lines = ichimoku.calculate(renko_bars);

        std::vector<Trade> trades;
        std::vector<StrategyLog> logs;
        Trade position;
        bool in_position = false;

        for (size_t i = 0; i < renko_bars.size(); ++i) {
            double c = renko_bars[i].close;
            double kijun = lines.kijun_sen[i];
            double sa = lines.span_a[i];
            double sb = lines.span_b[i];
            int available = i + 1;

            StrategyLog log;
            log.time = renko_bars[i].brick_time;
            log.close = c;
            log.kijun = kijun;
            log.available_renko = available;

            bool missing_data = std::isnan(kijun) || std::isnan(sa) || std::isnan(sb);
            if (missing_data) {
                std::ostringstream reason;
                reason << "Not enough history for Ichimoku -> ";
                if (std::isnan(kijun)) reason << "kijun: need " << kijun_len << ", have " << available << "; ";
                if (std::isnan(sa) || std::isnan(sb)) reason << "span_b: need " << span_b_len << ", have " << available;
                log.action = "skip";
                log.reason = reason.str();
                log.cloud_top = std::numeric_limits<double>::quiet_NaN();
                log.cloud_bottom = std::numeric_limits<double>::quiet_NaN();
                log.long_entry_signal = false;
                log.long_exit_signal = false;
                log.short_entry_signal = false;
                log.short_exit_signal = false;
                log.position = in_position ? position.direction : "";
                logs.push_back(log);
                continue;
            }

            log.cloud_top = std::max(sa, sb);
            log.cloud_bottom = std::min(sa, sb);
            bool inside_cloud = (c > log.cloud_bottom) && (c < log.cloud_top);

            bool long_entry = (c > kijun) && (c > log.cloud_top) && !inside_cloud;
            bool long_exit = (c < kijun) || (c < log.cloud_top);
            bool short_entry = (c < kijun) && (c < log.cloud_bottom) && !inside_cloud;
            bool short_exit = (c > kijun) || (c > log.cloud_bottom);

            log.long_entry_signal = long_entry;
            log.long_exit_signal = long_exit;
            log.short_entry_signal = short_entry;
            log.short_exit_signal = short_exit;

            if (!in_position) {
                if (long_entry) {
                    position.entry_time = renko_bars[i].brick_time;
                    position.entry_price = c;
                    position.direction = "long";
                    in_position = true;
                    log.action = "enter_long";
                    log.reason = "Renko close > kijun and > cloud_top";
                    std::cout << "Opening long trade: time=" << position.entry_time << " (" << unixToIST(position.entry_time) << ")"
                              << ", price=" << position.entry_price << std::endl;
                } else if (short_entry) {
                    position.entry_time = renko_bars[i].brick_time;
                    position.entry_price = c;
                    position.direction = "short";
                    in_position = true;
                    log.action = "enter_short";
                    log.reason = "Renko close < kijun and < cloud_bottom";
                    std::cout << "Opening short trade: time=" << position.entry_time << " (" << unixToIST(position.entry_time) << ")"
                              << ", price=" << position.entry_price << std::endl;
                } else {
                    log.action = "no_trade";
                    if (inside_cloud) {
                        log.reason = "No entry: price inside cloud";
                    } else if (c <= kijun || c <= log.cloud_top) {
                        log.reason = "No entry: price not above all required Ichimoku levels for long";
                    } else if (c >= kijun || c >= log.cloud_bottom) {
                        log.reason = "No entry: price not below all required Ichimoku levels for short";
                    } else {
                        log.reason = "No entry: conditions not met";
                    }
                }
            } else {
                if (position.direction == "long" && long_exit) {
                    position.exit_time = renko_bars[i].brick_time;
                    position.exit_price = c;
                    position.profit = c - position.entry_price;
                    trades.push_back(position);
                    log.action = "exit_long";
                    log.reason = "Renko closed below kijun or below cloud_top";
                    in_position = false;
                    std::cout << "Closing trade: entry_time=" << position.entry_time << " (" << unixToIST(position.entry_time) << ")"
                              << ", exit_time=" << position.exit_time << " (" << unixToIST(position.exit_time) << ")"
                              << ", direction=" << position.direction << ", profit=" << position.profit
                              << ", duration=" << (position.exit_time - position.entry_time) << " seconds" << std::endl;
                } else if (position.direction == "short" && short_exit) {
                    position.exit_time = renko_bars[i].brick_time;
                    position.exit_price = c;
                    position.profit = position.entry_price - c;
                    trades.push_back(position);
                    log.action = "exit_short";
                    log.reason = "Renko closed above kijun or above cloud_bottom";
                    in_position = false;
                    std::cout << "Closing trade: entry_time=" << position.entry_time << " (" << unixToIST(position.entry_time) << ")"
                              << ", exit_time=" << position.exit_time << " (" << unixToIST(position.exit_time) << ")"
                              << ", direction=" << position.direction << ", profit=" << position.profit
                              << ", duration=" << (position.exit_time - position.entry_time) << " seconds" << std::endl;
                } else {
                    log.action = (position.direction == "long") ? "hold_long" : "hold_short";
                    log.reason = "Exit conditions not met";
                }
            }

            log.position = in_position ? position.direction : "";
            logs.push_back(log);
        }

        if (in_position) {
            position.exit_time = renko_bars.back().brick_time;
            position.exit_price = renko_bars.back().close;
            position.profit = (position.direction == "long") ? (position.exit_price - position.entry_price) :
                                                              (position.entry_price - position.exit_price);
            trades.push_back(position);
            StrategyLog log;
            log.time = position.exit_time;
            log.action = "force_exit_" + position.direction;
            log.reason = "Closed at end of backtest";
            log.close = position.exit_price;
            log.kijun = lines.kijun_sen.back();
            log.cloud_top = std::max(lines.span_a.back(), lines.span_b.back());
            log.cloud_bottom = std::min(lines.span_a.back(), lines.span_b.back());
            log.long_entry_signal = false;
            log.long_exit_signal = false;
            log.short_entry_signal = false;
            log.short_exit_signal = false;
            log.position = "";
            log.available_renko = renko_bars.size();
            logs.push_back(log);
            std::cout << "Closing open trade at end: entry_time=" << position.entry_time << " (" << unixToIST(position.entry_time) << ")"
                      << ", exit_time=" << position.exit_time << " (" << unixToIST(position.exit_time) << ")"
                      << ", direction=" << position.direction << ", profit=" << position.profit
                      << ", duration=" << (position.exit_time - position.entry_time) << " seconds" << std::endl;
        }

        exportTradesToCSV(trades, "trades.csv");
        exportLogsToCSV(logs, "strategy_logs.csv");

        // Save renko_with_ichimoku.csv
        std::ofstream ri_file("renko_with_ichimoku.csv");
        if (ri_file) {
            ri_file << "brick_time,brick_time_ist,brick_start_time,brick_start_time_ist,src_open,src_high,src_low,src_close,open,high,low,close,dir,reversal,tenkan,kijun,span_a,span_b\n";
            for (size_t i = 0; i < renko_bars.size(); ++i) {
                ri_file << renko_bars[i].brick_time << "," << unixToIST(renko_bars[i].brick_time) << ","
                        << renko_bars[i].brick_start_time << "," << unixToIST(renko_bars[i].brick_start_time) << ","
                        << renko_bars[i].src_open << "," << renko_bars[i].src_high << "," << renko_bars[i].src_low << "," << renko_bars[i].src_close << ","
                        << renko_bars[i].open << "," << renko_bars[i].high << "," << renko_bars[i].low << "," << renko_bars[i].close << ","
                        << renko_bars[i].dir << "," << (renko_bars[i].reversal ? "true" : "false") << ","
                        << lines.tenkan_sen[i] << "," << lines.kijun_sen[i] << "," << lines.span_a[i] << "," << lines.span_b[i] << "\n";
            }
            ri_file.close();
            std::cout << "Exported renko with Ichimoku to renko_with_ichimoku.csv" << std::endl;
        }

        double net_profit = 0.0;
        for (const auto& trade : trades) {
            net_profit += trade.profit;
        }
        std::cout << "\n========= Backtest Summary =========\n";
        std::cout << "Renko bricks: " << renko_bars.size() << "\n";
        std::cout << "Trades: " << trades.size() << "\n";
        std::cout << "Net Profit (pts): " << net_profit << "\n";
        std::cout << "Saved: renko_with_ichimoku.csv, trades.csv, strategy_logs.csv\n";
        std::cout << "====================================\n";

        std::cout << "Last 15 Renko bricks (preview):\n";
        size_t start = (renko_bars.size() > 15) ? renko_bars.size() - 15 : 0;
        for (size_t i = start; i < renko_bars.size(); ++i) {
            std::string dir = renko_bars[i].dir == 1 ? "UP" : (renko_bars[i].dir == -1 ? "DOWN" : "NONE");
            std::string type = renko_bars[i].reversal ? "Reversal" : "Normal";
            std::cout << "[" << unixToIST(renko_bars[i].brick_time) << "] " << dir << " | " << type << " | "
                      << renko_bars[i].open << " -> " << renko_bars[i].close << "\n";
        }

        return trades;
    }

    void exportTradesToCSV(const std::vector<Trade>& trades, const std::string& filename) {
        std::ofstream file(filename);
        if (!file) {
            std::cerr << "Failed to open " << filename << " for writing" << std::endl;
            return;
        }
        file << "entry_time,entry_time_ist,entry_price,direction,exit_time,exit_time_ist,exit_price,profit\n";
        for (const auto& trade : trades) {
            file << trade.entry_time << "," << unixToIST(trade.entry_time) << ","
                 << trade.entry_price << "," << trade.direction << ","
                 << trade.exit_time << "," << unixToIST(trade.exit_time) << ","
                 << trade.exit_price << "," << trade.profit << "\n";
        }
        file.close();
        std::cout << "Exported trades to " << filename << std::endl;
    }

    void exportLogsToCSV(const std::vector<StrategyLog>& logs, const std::string& filename) {
        std::ofstream file(filename);
        if (!file) {
            std::cerr << "Failed to open " << filename << " for writing" << std::endl;
            return;
        }
        file << "time,time_ist,action,reason,close,kijun,cloud_top,cloud_bottom,long_entry_signal,long_exit_signal,short_entry_signal,short_exit_signal,position,available_renko\n";
        for (const auto& log : logs) {
            file << log.time << "," << unixToIST(log.time) << ","
                 << log.action << "," << "\"" << log.reason << "\","
                 << log.close << "," << log.kijun << "," << log.cloud_top << "," << log.cloud_bottom << ","
                 << (log.long_entry_signal ? "true" : "false") << "," << (log.long_exit_signal ? "true" : "false") << ","
                 << (log.short_entry_signal ? "true" : "false") << "," << (log.short_exit_signal ? "true" : "false") << ","
                 << log.position << "," << log.available_renko << "\n";
        }
        file.close();
        std::cout << "Exported strategy logs to " << filename << std::endl;
    }
};

#endif