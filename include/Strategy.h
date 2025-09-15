#ifndef STRATEGY_H
#define STRATEGY_H

#include <vector>
#include <fstream>
#include <iomanip>
#include <ctime>
#include <iostream>
#include <limits>
#include <cstring>
#include "OHLC.h"
#include "Renko.h"
#include "Ichimoku.h"
#include "Trade.h"

struct StrategyLog {
    long time;
    double close;
    double kijun;
    double cloud_top;
    double cloud_bottom;
    int available_renko;
    bool long_entry_signal;
    bool long_exit_signal;
    bool short_entry_signal;
    bool short_exit_signal;
    char action[32];
    char reason[128];
    char position[16];

    StrategyLog() : time(0), close(0.0), kijun(std::numeric_limits<double>::quiet_NaN()),
                    cloud_top(std::numeric_limits<double>::quiet_NaN()), cloud_bottom(std::numeric_limits<double>::quiet_NaN()),
                    available_renko(0), long_entry_signal(false), long_exit_signal(false),
                    short_entry_signal(false), short_exit_signal(false) {
        action[0] = '\0';
        reason[0] = '\0';
        position[0] = '\0';
    }
};

class Strategy {
private:
    double brick_size, reversal_size;
    int tenkan_len, kijun_len, span_b_len, displacement;

    std::string unixToIST(long unix_time) {
        std::cout << "  unixToIST: Converting timestamp " << unix_time << std::endl;
        unix_time += 19800; // IST = UTC+5:30
        std::time_t t = static_cast<std::time_t>(unix_time);
        std::tm* tm = std::localtime(&t);
        if (!tm) {
            std::cout << "  unixToIST: Failed to convert timestamp " << unix_time << std::endl;
            return "INVALID_TIME";
        }
        char buffer[32];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S IST", tm);
        std::cout << "  unixToIST: Success, result=" << buffer << std::endl;
        return std::string(buffer);
    }

public:
    Strategy(double bs = 40.0, double rs = 80.0, int t = 5, int k = 26, int sb = 52, int d = 26)
        : brick_size(bs), reversal_size(rs), tenkan_len(t), kijun_len(k), span_b_len(sb), displacement(d) {}

    std::vector<Trade> backtest(const std::vector<OHLC>& ohlc_data) {
        std::cout << "Starting backtest" << std::endl;
        RenkoBuilder renko(brick_size, reversal_size);
        renko.buildFromOHLC(ohlc_data);
        auto renko_bars = renko.getRenkoBars();
        if (renko_bars.empty()) {
            std::cerr << "No Renko bars generated. Exiting backtest." << std::endl;
            return {};
        }
        std::cout << "Starting backtest with " << renko_bars.size() << " Renko bars" << std::endl;

        // Export renko bars immediately
        renko.exportToCSV("renko_with_ichimoku.csv");

        Ichimoku ichimoku(tenkan_len, kijun_len, span_b_len, displacement);
        auto lines = ichimoku.calculate(renko_bars);
        if (lines.tenkan_sen.size() != renko_bars.size() ||
            lines.kijun_sen.size() != renko_bars.size() ||
            lines.span_a.size() != renko_bars.size() ||
            lines.span_b.size() != renko_bars.size()) {
            std::cerr << "Ichimoku lines size mismatch: tenkan_sen.size=" << lines.tenkan_sen.size()
                      << ", kijun_sen.size=" << lines.kijun_sen.size()
                      << ", span_a.size=" << lines.span_a.size()
                      << ", span_b.size=" << lines.span_b.size()
                      << ", renko_bars.size=" << renko_bars.size() << std::endl;
            return {};
        }

        std::vector<Trade> trades;
        std::vector<StrategyLog> logs;
        logs.reserve(renko_bars.size());
        Trade position;
        bool in_position = false;

        for (size_t i = 0; i < renko_bars.size(); ++i) {
            std::cout << "Processing Renko bar " << i << ": time=" << renko_bars[i].brick_time
                      << " (" << unixToIST(renko_bars[i].brick_time) << "), close=" << renko_bars[i].close << std::endl;

            if (i >= lines.tenkan_sen.size() || i >= lines.kijun_sen.size() ||
                i >= lines.span_a.size() || i >= lines.span_b.size()) {
                std::cerr << "Index " << i << " exceeds Ichimoku lines size" << std::endl;
                continue;
            }

            StrategyLog log;
            log.time = renko_bars[i].brick_time;
            log.close = renko_bars[i].close;
            log.kijun = lines.kijun_sen[i];
            log.available_renko = static_cast<int>(i + 1);

            std::cout << "  Setting log fields: kijun=" << log.kijun << std::endl;

            bool missing_data = std::isnan(log.kijun) || std::isnan(lines.span_a[i]) || std::isnan(lines.span_b[i]);
            if (missing_data) {
                std::strncpy(log.action, "skip", sizeof(log.action) - 1);
                log.action[sizeof(log.action) - 1] = '\0';
                if (i == 0) {
                    // Skip string construction for bar 0 to isolate crash
                    std::strncpy(log.reason, "Skipped for testing", sizeof(log.reason) - 1);
                    log.reason[sizeof(log.reason) - 1] = '\0';
                } else {
                    char reason[128] = "Not enough history for Ichimoku -> ";
                    if (std::isnan(log.kijun)) {
                        char buf[32];
                        std::snprintf(buf, sizeof(buf), "kijun: need %d, have %d", kijun_len, log.available_renko);
                        std::strncat(reason, buf, sizeof(reason) - std::strlen(reason) - 1);
                    }
                    if (std::isnan(lines.span_a[i]) || std::isnan(lines.span_b[i])) {
                        if (reason[std::strlen(reason) - 1] != '>') {
                            std::strncat(reason, "; ", sizeof(reason) - std::strlen(reason) - 1);
                        }
                        char buf[32];
                        std::snprintf(buf, sizeof(buf), "span_b: need %d, have %d", span_b_len, log.available_renko);
                        std::strncat(reason, buf, sizeof(reason) - std::strlen(reason) - 1);
                    }
                    std::strncpy(log.reason, reason, sizeof(log.reason) - 1);
                    log.reason[sizeof(log.reason) - 1] = '\0';
                }
                log.cloud_top = std::numeric_limits<double>::quiet_NaN();
                log.cloud_bottom = std::numeric_limits<double>::quiet_NaN();
                std::strncpy(log.position, in_position ? position.direction.c_str() : "", sizeof(log.position) - 1);
                log.position[sizeof(log.position) - 1] = '\0';
                std::cout << "  Skipping: " << log.reason << std::endl;
                std::cout << "  Before emplace_back: action=" << log.action << ", reason=" << log.reason << std::endl;
                logs.emplace_back(log);
                std::cout << "  After emplace_back" << std::endl;
                continue;
            }

            log.cloud_top = std::max(lines.span_a[i], lines.span_b[i]);
            log.cloud_bottom = std::min(lines.span_a[i], lines.span_b[i]);
            bool inside_cloud = (log.close > log.cloud_bottom) && (log.close < log.cloud_top);

            std::cout << "  Cloud: top=" << log.cloud_top << ", bottom=" << log.cloud_bottom << ", inside_cloud=" << inside_cloud << std::endl;

            bool long_entry = (log.close > log.kijun) && (log.close > log.cloud_top) && !inside_cloud;
            bool long_exit = (log.close < log.kijun) || (log.close < log.cloud_top);
            bool short_entry = (log.close < log.kijun) && (log.close < log.cloud_bottom) && !inside_cloud;
            bool short_exit = (log.close > log.kijun) || (log.close > log.cloud_bottom);

            log.long_entry_signal = long_entry;
            log.long_exit_signal = long_exit;
            log.short_entry_signal = short_entry;
            log.short_exit_signal = short_exit;

            std::cout << "  Signals: long_entry=" << long_entry << ", long_exit=" << long_exit
                      << ", short_entry=" << short_entry << ", short_exit=" << short_exit << std::endl;

            if (!in_position) {
                if (long_entry) {
                    position.entry_time = renko_bars[i].brick_time;
                    position.entry_price = log.close;
                    position.direction = "long";
                    in_position = true;
                    std::strncpy(log.action, "enter_long", sizeof(log.action) - 1);
                    log.action[sizeof(log.action) - 1] = '\0';
                    std::strncpy(log.reason, "Renko close > kijun and > cloud_top", sizeof(log.reason) - 1);
                    log.reason[sizeof(log.reason) - 1] = '\0';
                    std::cout << "Opening long trade: time=" << position.entry_time << " (" << unixToIST(position.entry_time) << ")"
                              << ", price=" << position.entry_price << std::endl;
                } else if (short_entry) {
                    position.entry_time = renko_bars[i].brick_time;
                    position.entry_price = log.close;
                    position.direction = "short";
                    in_position = true;
                    std::strncpy(log.action, "enter_short", sizeof(log.action) - 1);
                    log.action[sizeof(log.action) - 1] = '\0';
                    std::strncpy(log.reason, "Renko close < kijun and < cloud_bottom", sizeof(log.reason) - 1);
                    log.reason[sizeof(log.reason) - 1] = '\0';
                    std::cout << "Opening short trade: time=" << position.entry_time << " (" << unixToIST(position.entry_time) << ")"
                              << ", price=" << position.entry_price << std::endl;
                } else {
                    std::strncpy(log.action, "no_trade", sizeof(log.action) - 1);
                    log.action[sizeof(log.action) - 1] = '\0';
                    if (inside_cloud) {
                        std::strncpy(log.reason, "No entry: price inside cloud", sizeof(log.reason) - 1);
                    } else if (log.close <= log.kijun || log.close <= log.cloud_top) {
                        std::strncpy(log.reason, "No entry: price not above all required Ichimoku levels for long", sizeof(log.reason) - 1);
                    } else if (log.close >= log.kijun || log.close >= log.cloud_bottom) {
                        std::strncpy(log.reason, "No entry: price not below all required Ichimoku levels for short", sizeof(log.reason) - 1);
                    } else {
                        std::strncpy(log.reason, "No entry: conditions not met", sizeof(log.reason) - 1);
                    }
                    log.reason[sizeof(log.reason) - 1] = '\0';
                    std::cout << "  No trade: " << log.reason << std::endl;
                }
            } else {
                if (position.direction == "long" && long_exit) {
                    position.exit_time = renko_bars[i].brick_time;
                    position.exit_price = log.close;
                    position.profit = log.close - position.entry_price;
                    long duration = position.exit_time - position.entry_time;
                    if (duration < 0) {
                        std::cerr << "Warning: Negative duration detected: entry_time=" << position.entry_time
                                  << " (" << unixToIST(position.entry_time) << "), exit_time=" << position.exit_time
                                  << " (" << unixToIST(position.exit_time) << ")" << std::endl;
                    }
                    trades.push_back(position);
                    std::strncpy(log.action, "exit_long", sizeof(log.action) - 1);
                    log.action[sizeof(log.action) - 1] = '\0';
                    std::strncpy(log.reason, "Renko closed below kijun or below cloud_top", sizeof(log.reason) - 1);
                    log.reason[sizeof(log.reason) - 1] = '\0';
                    in_position = false;
                    std::cout << "Closing trade: entry_time=" << position.entry_time << " (" << unixToIST(position.entry_time) << ")"
                              << ", exit_time=" << position.exit_time << " (" << unixToIST(position.exit_time) << ")"
                              << ", direction=" << position.direction << ", profit=" << position.profit
                              << ", duration=" << duration << " seconds" << std::endl;
                } else if (position.direction == "short" && short_exit) {
                    position.exit_time = renko_bars[i].brick_time;
                    position.exit_price = log.close;
                    position.profit = position.entry_price - log.close;
                    long duration = position.exit_time - position.entry_time;
                    if (duration < 0) {
                        std::cerr << "Warning: Negative duration detected: entry_time=" << position.entry_time
                                  << " (" << unixToIST(position.entry_time) << "), exit_time=" << position.exit_time
                                  << " (" << unixToIST(position.exit_time) << ")" << std::endl;
                    }
                    trades.push_back(position);
                    std::strncpy(log.action, "exit_short", sizeof(log.action) - 1);
                    log.action[sizeof(log.action) - 1] = '\0';
                    std::strncpy(log.reason, "Renko closed above kijun or above cloud_bottom", sizeof(log.reason) - 1);
                    log.reason[sizeof(log.reason) - 1] = '\0';
                    in_position = false;
                    std::cout << "Closing trade: entry_time=" << position.entry_time << " (" << unixToIST(position.entry_time) << ")"
                              << ", exit_time=" << position.exit_time << " (" << unixToIST(position.exit_time) << ")"
                              << ", direction=" << position.direction << ", profit=" << position.profit
                              << ", duration=" << duration << " seconds" << std::endl;
                } else {
                    std::strncpy(log.action, (position.direction == "long") ? "hold_long" : "hold_short", sizeof(log.action) - 1);
                    log.action[sizeof(log.action) - 1] = '\0';
                    std::strncpy(log.reason, "Exit conditions not met", sizeof(log.reason) - 1);
                    log.reason[sizeof(log.reason) - 1] = '\0';
                    std::cout << "  Holding position: " << log.action << std::endl;
                }
            }

            std::strncpy(log.position, in_position ? position.direction.c_str() : "", sizeof(log.position) - 1);
            log.position[sizeof(log.position) - 1] = '\0';
            std::cout << "  Before emplace_back: action=" << log.action << ", reason=" << log.reason << std::endl;
            logs.emplace_back(log);
            std::cout << "  After emplace_back" << std::endl;
        }

        if (in_position) {
            position.exit_time = renko_bars.back().brick_time;
            position.exit_price = renko_bars.back().close;
            position.profit = (position.direction == "long") ? (position.exit_price - position.entry_price) :
                                                              (position.entry_price - position.exit_price);
            long duration = position.exit_time - position.entry_time;
            if (duration < 0) {
                std::cerr << "Warning: Negative duration detected: entry_time=" << position.entry_time
                          << " (" << unixToIST(position.entry_time) << "), exit_time=" << position.exit_time
                          << " (" << unixToIST(position.exit_time) << ")" << std::endl;
            }
            trades.push_back(position);
            StrategyLog log;
            log.time = position.exit_time;
            log.close = position.exit_price;
            log.kijun = lines.kijun_sen.back();
            log.cloud_top = std::max(lines.span_a.back(), lines.span_b.back());
            log.cloud_bottom = std::min(lines.span_a.back(), lines.span_b.back());
            log.long_entry_signal = false;
            log.long_exit_signal = false;
            log.short_entry_signal = false;
            log.short_exit_signal = false;
            log.available_renko = static_cast<int>(renko_bars.size());
            std::strncpy(log.action, ("force_exit_" + position.direction).c_str(), sizeof(log.action) - 1);
            log.action[sizeof(log.action) - 1] = '\0';
            std::strncpy(log.reason, "Closed at end of backtest", sizeof(log.reason) - 1);
            log.reason[sizeof(log.reason) - 1] = '\0';
            std::strncpy(log.position, "", sizeof(log.position) - 1);
            log.position[sizeof(log.position) - 1] = '\0';
            std::cout << "Closing open trade at end: entry_time=" << position.entry_time << " (" << unixToIST(position.entry_time) << ")"
                      << ", exit_time=" << position.exit_time << " (" << unixToIST(position.exit_time) << ")"
                      << ", direction=" << position.direction << ", profit=" << position.profit
                      << ", duration=" << duration << " seconds" << std::endl;
            std::cout << "  Before emplace_back (force_exit): action=" << log.action << ", reason=" << log.reason << std::endl;
            logs.emplace_back(log);
            std::cout << "  After emplace_back (force_exit)" << std::endl;
        }

        std::cout << "Exporting trades and logs" << std::endl;
        exportTradesToCSV(trades, "trades.csv");
        exportLogsToCSV(logs, "strategy_logs.csv");

        // Save renko_with_ichimoku.csv with Ichimoku data
        std::ofstream ri_file("renko_with_ichimoku.csv");
        if (!ri_file) {
            std::cerr << "Failed to open renko_with_ichimoku.csv for writing" << std::endl;
        } else {
            ri_file << "brick_time,brick_time_ist,brick_start_time,brick_start_time_ist,src_open,src_high,src_low,src_close,open,high,low,close,dir,reversal,tenkan,kijun,span_a,span_b\n";
            for (size_t i = 0; i < renko_bars.size(); ++i) {
                if (i >= lines.tenkan_sen.size() || i >= lines.kijun_sen.size() ||
                    i >= lines.span_a.size() || i >= lines.span_b.size()) {
                    std::cerr << "Index " << i << " exceeds Ichimoku lines size in renko_with_ichimoku.csv" << std::endl;
                    continue;
                }
                ri_file << renko_bars[i].brick_time << "," << unixToIST(renko_bars[i].brick_time) << ","
                        << renko_bars[i].brick_start_time << "," << unixToIST(renko_bars[i].brick_start_time) << ","
                        << renko_bars[i].src_open << "," << renko_bars[i].src_high << "," << renko_bars[i].src_low << "," << renko_bars[i].src_close << ","
                        << renko_bars[i].open << "," << renko_bars[i].high << "," << renko_bars[i].low << "," << renko_bars[i].close << ","
                        << renko_bars[i].dir << "," << (renko_bars[i].reversal ? "true" : "false") << ","
                        << (std::isnan(lines.tenkan_sen[i]) ? "NaN" : std::to_string(lines.tenkan_sen[i])) << ","
                        << (std::isnan(lines.kijun_sen[i]) ? "NaN" : std::to_string(lines.kijun_sen[i])) << ","
                        << (std::isnan(lines.span_a[i]) ? "NaN" : std::to_string(lines.span_a[i])) << ","
                        << (std::isnan(lines.span_b[i]) ? "NaN" : std::to_string(lines.span_b[i])) << "\n";
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
                 << log.close << "," << (std::isnan(log.kijun) ? "NaN" : std::to_string(log.kijun)) << ","
                 << (std::isnan(log.cloud_top) ? "NaN" : std::to_string(log.cloud_top)) << ","
                 << (std::isnan(log.cloud_bottom) ? "NaN" : std::to_string(log.cloud_bottom)) << ","
                 << (log.long_entry_signal ? "true" : "false") << "," << (log.long_exit_signal ? "true" : "false") << ","
                 << (log.short_entry_signal ? "true" : "false") << "," << (log.short_exit_signal ? "true" : "false") << ","
                 << log.position << "," << log.available_renko << "\n";
        }
        file.close();
        std::cout << "Exported strategy logs to " << filename << std::endl;
    }
};

#endif