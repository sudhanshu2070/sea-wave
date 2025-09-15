#ifndef RENKO_H
#define RENKO_H

#include <vector>
#include <fstream>
#include <iomanip>
#include <ctime>
#include <iostream>
#include "OHLC.h"

struct RenkoBar {
    long brick_time;
    long brick_start_time;
    double src_open, src_high, src_low, src_close;
    double open, high, low, close;
    int dir; // 1 for up, -1 for down
    bool reversal;
};

class RenkoBuilder {
private:
    double brick_size;
    double reversal_size;
    std::vector<RenkoBar> bars;

    std::string unixToIST(long unix_time) {
        unix_time += 19800;
        std::time_t t = static_cast<std::time_t>(unix_time);
        std::tm* tm = std::localtime(&t);
        std::ostringstream oss;
        oss << std::put_time(tm, "%Y-%m-%d %H:%M:%S") << " IST";
        return oss.str();
    }

public:
    RenkoBuilder(double size, double rev_size) : brick_size(size), reversal_size(rev_size) {}

    void addPrice(double price, long timestamp, double src_open, double src_high, double src_low, double src_close) {
        if (bars.empty()) {
            RenkoBar bar;
            bar.brick_time = timestamp;
            bar.brick_start_time = timestamp;
            bar.src_open = src_open;
            bar.src_high = src_high;
            bar.src_low = src_low;
            bar.src_close = src_close;
            bar.open = bar.high = bar.low = bar.close = price;
            bar.dir = 0;
            bar.reversal = false;
            bars.push_back(bar);
            return;
        }

        RenkoBar& last_bar = bars.back();
        double diff = price - last_bar.close;
        bool progressed = true;

        while (progressed) {
            progressed = false;
            RenkoBar new_bar;
            new_bar.brick_time = timestamp;
            new_bar.src_open = src_open;
            new_bar.src_high = src_high;
            new_bar.src_low = src_low;
            new_bar.src_close = src_close;

            if (last_bar.dir == 0) {
                if (std::abs(diff) >= brick_size) {
                    new_bar.dir = (diff > 0) ? 1 : -1;
                    new_bar.open = last_bar.close;
                    new_bar.close = new_bar.open + new_bar.dir * brick_size;
                    new_bar.high = std::max(new_bar.open, new_bar.close);
                    new_bar.low = std::min(new_bar.open, new_bar.close);
                    new_bar.brick_start_time = timestamp;
                    new_bar.reversal = false;
                    bars.push_back(new_bar);
                    last_bar = new_bar;
                    diff = price - new_bar.close;
                    progressed = true;
                }
            } else if (last_bar.dir == 1) {
                if (price >= last_bar.close + brick_size) {
                    new_bar.open = last_bar.close;
                    new_bar.close = new_bar.open + brick_size;
                    new_bar.high = new_bar.close;
                    new_bar.low = new_bar.open;
                    new_bar.dir = 1;
                    new_bar.brick_start_time = last_bar.brick_start_time;
                    new_bar.reversal = false;
                    bars.push_back(new_bar);
                    last_bar = new_bar;
                    diff = price - new_bar.close;
                    progressed = true;
                } else if (price <= last_bar.close - reversal_size) {
                    new_bar.open = last_bar.close;
                    new_bar.close = new_bar.open - reversal_size;
                    new_bar.high = new_bar.open;
                    new_bar.low = new_bar.close;
                    new_bar.dir = -1;
                    new_bar.brick_start_time = timestamp;
                    new_bar.reversal = true;
                    bars.push_back(new_bar);
                    last_bar = new_bar;
                    diff = price - new_bar.close;
                    progressed = true;
                }
            } else {
                if (price <= last_bar.close - brick_size) {
                    new_bar.open = last_bar.close;
                    new_bar.close = new_bar.open - brick_size;
                    new_bar.high = new_bar.open;
                    new_bar.low = new_bar.close;
                    new_bar.dir = -1;
                    new_bar.brick_start_time = last_bar.brick_start_time;
                    new_bar.reversal = false;
                    bars.push_back(new_bar);
                    last_bar = new_bar;
                    diff = price - new_bar.close;
                    progressed = true;
                } else if (price >= last_bar.close + reversal_size) {
                    new_bar.open = last_bar.close;
                    new_bar.close = new_bar.open + reversal_size;
                    new_bar.high = new_bar.close;
                    new_bar.low = new_bar.open;
                    new_bar.dir = 1;
                    new_bar.brick_start_time = timestamp;
                    new_bar.reversal = true;
                    bars.push_back(new_bar);
                    last_bar = new_bar;
                    diff = price - new_bar.close;
                    progressed = true;
                }
            }
        }
    }

    void buildFromOHLC(const std::vector<OHLC>& ohlc_data) {
        bars.clear();
        for (const auto& candle : ohlc_data) {
            addPrice(candle.close, candle.time, candle.open, candle.high, candle.low, candle.close);
        }
        std::cout << "Built " << bars.size() << " Renko bricks" << std::endl;
    }

    std::vector<RenkoBar> getRenkoBars() const {
        return bars;
    }

    void exportToCSV(const std::string& filename) {
        std::ofstream file(filename);
        if (!file) {
            std::cerr << "Failed to open " << filename << " for writing" << std::endl;
            return;
        }
        file << "brick_time,brick_time_ist,brick_start_time,brick_start_time_ist,src_open,src_high,src_low,src_close,open,high,low,close,dir,reversal\n";
        for (const auto& bar : bars) {
            file << bar.brick_time << "," << unixToIST(bar.brick_time) << ","
                 << bar.brick_start_time << "," << unixToIST(bar.brick_start_time) << ","
                 << bar.src_open << "," << bar.src_high << "," << bar.src_low << "," << bar.src_close << ","
                 << bar.open << "," << bar.high << "," << bar.low << "," << bar.close << ","
                 << bar.dir << "," << (bar.reversal ? "true" : "false") << "\n";
        }
        file.close();
        std::cout << "Exported Renko bars to " << filename << std::endl;
    }
};

#endif