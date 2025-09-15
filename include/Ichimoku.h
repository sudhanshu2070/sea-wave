#ifndef ICHIMOKU_H
#define ICHIMOKU_H

#include <vector>
#include <algorithm>
#include <limits>
#include <iostream>
#include "Renko.h"

struct IchimokuLines {
    std::vector<double> tenkan_sen;
    std::vector<double> kijun_sen;
    std::vector<double> span_a;
    std::vector<double> span_b;
};

class Ichimoku {
private:
    int tenkan_len, kijun_len, span_b_len, displacement;

    double donchian_mid(const std::vector<double>& prices, size_t index, int length) {
        if (index < length - 1 || index >= prices.size()) {
            std::cerr << "donchian_mid: Invalid index " << index << " for length " << length 
                      << ", prices.size=" << prices.size() << std::endl;
            return std::numeric_limits<double>::quiet_NaN();
        }
        auto start = prices.begin() + index - length + 1;
        auto end = prices.begin() + index + 1;
        if (start < prices.begin() || end > prices.end()) {
            std::cerr << "donchian_mid: Invalid range for index " << index << std::endl;
            return std::numeric_limits<double>::quiet_NaN();
        }
        double max_val = *std::max_element(start, end);
        double min_val = *std::min_element(start, end);
        return (max_val + min_val) / 2.0;
    }

public:
    Ichimoku(int t = 5, int k = 26, int sb = 52, int d = 26)
        : tenkan_len(t), kijun_len(k), span_b_len(sb), displacement(d) {}

    IchimokuLines calculate(const std::vector<RenkoBar>& bars) {
        IchimokuLines lines;
        if (bars.empty()) {
            std::cerr << "Ichimoku::calculate: Empty Renko bars" << std::endl;
            return lines;
        }

        lines.tenkan_sen.resize(bars.size(), std::numeric_limits<double>::quiet_NaN());
        lines.kijun_sen.resize(bars.size(), std::numeric_limits<double>::quiet_NaN());
        lines.span_a.resize(bars.size(), std::numeric_limits<double>::quiet_NaN());
        lines.span_b.resize(bars.size(), std::numeric_limits<double>::quiet_NaN());

        std::vector<double> closes(bars.size());
        for (size_t i = 0; i < bars.size(); ++i) {
            closes[i] = bars[i].close;
        }

        for (size_t i = 0; i < bars.size(); ++i) {
            if (i >= tenkan_len - 1) {
                lines.tenkan_sen[i] = donchian_mid(closes, i, tenkan_len);
            }
            if (i >= kijun_len - 1) {
                lines.kijun_sen[i] = donchian_mid(closes, i, kijun_len);
            }
            if (i >= span_b_len - 1) {
                lines.span_b[i] = donchian_mid(closes, i, span_b_len);
            }
            if (!std::isnan(lines.tenkan_sen[i]) && !std::isnan(lines.kijun_sen[i])) {
                lines.span_a[i] = (lines.tenkan_sen[i] + lines.kijun_sen[i]) / 2.0;
            }
        }

        std::cout << "Ichimoku calculated: tenkan_sen.size=" << lines.tenkan_sen.size() 
                  << ", kijun_sen.size=" << lines.kijun_sen.size() 
                  << ", span_a.size=" << lines.span_a.size() 
                  << ", span_b.size=" << lines.span_b.size() << std::endl;
        return lines;
    }
};

#endif