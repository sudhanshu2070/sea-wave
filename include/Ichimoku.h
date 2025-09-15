#ifndef ICHIMOKU_H
#define ICHIMOKU_H

#include <vector>
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
        if (index < length - 1) return std::numeric_limits<double>::quiet_NaN();
        double max_val = *std::max_element(prices.begin() + index - length + 1, prices.begin() + index + 1);
        double min_val = *std::min_element(prices.begin() + index - length + 1, prices.begin() + index + 1);
        return (max_val + min_val) / 2.0;
    }

public:
    Ichimoku(int t = 5, int k = 26, int sb = 52, int d = 26)
        : tenkan_len(t), kijun_len(k), span_b_len(sb), displacement(d) {}

    IchimokuLines calculate(const std::vector<RenkoBar>& bars) {
        IchimokuLines lines;
        lines.tenkan_sen.resize(bars.size());
        lines.kijun_sen.resize(bars.size());
        lines.span_a.resize(bars.size());
        lines.span_b.resize(bars.size());

        std::vector<double> closes(bars.size());
        for (size_t i = 0; i < bars.size(); ++i) {
            closes[i] = bars[i].close;
        }

        for (size_t i = 0; i < bars.size(); ++i) {
            lines.tenkan_sen[i] = donchian_mid(closes, i, tenkan_len);
            lines.kijun_sen[i] = donchian_mid(closes, i, kijun_len);
            lines.span_b[i] = donchian_mid(closes, i, span_b_len);
            lines.span_a[i] = (lines.tenkan_sen[i] + lines.kijun_sen[i]) / 2.0;
        }

        return lines;
    }
};

#endif