// Ichimoku.h
// Ichimoku indicator calculator
#ifndef ICHIMOKU_H
#define ICHIMOKU_H

#include <vector>
#include "OHLC.h"

struct IchimokuLines {
    std::vector<double> tenkan_sen;
    std::vector<double> kijun_sen;
    std::vector<double> senkou_span_a;
    std::vector<double> senkou_span_b;
    std::vector<double> chikou_span;
};

class Ichimoku {
private:
    int conv_period;
    int base_period;
    int span_b_period;
    int displacement;

    double highestHigh(const std::vector<OHLC>& data, size_t start, size_t end) {
        double max = data[start].high;
        for (size_t i = start + 1; i <= end; ++i) {
            if (data[i].high > max) max = data[i].high;
        }
        return max;
    }

    double lowestLow(const std::vector<OHLC>& data, size_t start, size_t end) {
        double min = data[start].low;
        for (size_t i = start + 1; i <= end; ++i) {
            if (data[i].low < min) min = data[i].low;
        }
        return min;
    }

public:
    Ichimoku(int conv = 5, int base = 26, int spanb = 52, int disp = 26)
        : conv_period(conv), base_period(base), span_b_period(spanb), displacement(disp) {}

    IchimokuLines calculate(const std::vector<OHLC>& data) {
        size_t n = data.size();
        IchimokuLines lines;
        lines.tenkan_sen.resize(n, 0.0);
        lines.kijun_sen.resize(n, 0.0);
        lines.senkou_span_a.resize(n + displacement, 0.0);  // Extended for shift
        lines.senkou_span_b.resize(n + displacement, 0.0);
        lines.chikou_span.resize(n, 0.0);

        for (size_t i = 0; i < n; ++i) {
            if (i >= conv_period - 1) {
                size_t start = i - conv_period + 1;
                double hh = highestHigh(data, start, i);
                double ll = lowestLow(data, start, i);
                lines.tenkan_sen[i] = (hh + ll) / 2.0;
            }

            if (i >= base_period - 1) {
                size_t start = i - base_period + 1;
                double hh = highestHigh(data, start, i);
                double ll = lowestLow(data, start, i);
                lines.kijun_sen[i] = (hh + ll) / 2.0;
            }

            if (i >= span_b_period - 1) {
                size_t start = i - span_b_period + 1;
                double hh = highestHigh(data, start, i);
                double ll = lowestLow(data, start, i);
                lines.senkou_span_b[i + displacement] = (hh + ll) / 2.0;
            }

            if (i >= base_period - 1) {
                lines.senkou_span_a[i + displacement] = (lines.tenkan_sen[i] + lines.kijun_sen[i]) / 2.0;
            }

            if (i + displacement < n) {
                lines.chikou_span[i + displacement] = data[i].close;
            }
        }

        // Trim extended vectors to original size if needed, but keep for forward shift
        lines.senkou_span_a.resize(n);
        lines.senkou_span_b.resize(n);

        return lines;
    }
};

#endif