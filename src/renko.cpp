#include "../include/backtest/renko.hpp"
#include <cmath>
#include <algorithm>

namespace backtest {

std::vector<RenkoBrick> build_renko(
    const std::vector<Candle>& candles,
    double brick_size,
    double reversal_size
) {
    std::vector<RenkoBrick> rows;
    double last_price = 0.0;
    int last_dir = 0; // 0 = none, +1 = up, -1 = down
    long long trend_start_time = 0;

    for (const auto& row : candles) {
        double price = row.close;
        long long ts = row.time;
        double src_o = row.open;
        double src_h = row.high;
        double src_l = row.low;
        double src_c = row.close;

        if (last_price == 0.0) {
            last_price = price;
            trend_start_time = ts;
            continue;
        }

        bool progressed = true;
        while (progressed) {
            progressed = false;
            if (last_dir == 0) {
                double diff = price - last_price;
                if (std::abs(diff) >= brick_size) {
                    int dir_ = (diff > 0) ? +1 : -1;
                    double new_close = last_price + dir_ * brick_size;
                    double o = last_price;
                    double h = std::max(o, new_close);
                    double l = std::min(o, new_close);
                    rows.push_back({ts, trend_start_time, src_o, src_h, src_l, src_c, o, h, l, new_close, dir_, false});
                    last_price = new_close;
                    last_dir = dir_;
                    progressed = true;
                }
            } else if (last_dir == +1) {
                if (price >= last_price + brick_size) {
                    double new_close = last_price + brick_size;
                    double o = last_price;
                    double h = new_close;
                    double l = o;
                    rows.push_back({ts, trend_start_time, src_o, src_h, src_l, src_c, o, h, l, new_close, +1, false});
                    last_price = new_close;
                    progressed = true;
                } else if (price <= last_price - reversal_size) {
                    double new_close = last_price - reversal_size;
                    double o = last_price;
                    double h = o;
                    double l = new_close;
                    rows.push_back({ts, ts, src_o, src_h, src_l, src_c, o, h, l, new_close, -1, true});
                    last_price = new_close;
                    last_dir = -1;
                }
            } else { // last_dir == -1
                if (price <= last_price - brick_size) {
                    double new_close = last_price - brick_size;
                    double o = last_price;
                    double h = o;
                    double l = new_close;
                    rows.push_back({ts, trend_start_time, src_o, src_h, src_l, src_c, o, h, l, new_close, -1, false});
                    last_price = new_close;
                    progressed = true;
                } else if (price >= last_price + reversal_size) {
                    double new_close = last_price + reversal_size;
                    double o = last_price;
                    double h = new_close;
                    double l = o;
                    rows.push_back({ts, ts, src_o, src_h, src_l, src_c, o, h, l, new_close, +1, true});
                    last_price = new_close;
                    last_dir = +1;
                }
            }
        }
    }

    // Sort by brick_time (should already be, but ensure)
    std::sort(rows.begin(), rows.end(), [](const RenkoBrick& a, const RenkoBrick& b) {
        return a.brick_time < b.brick_time;
    });

    return rows;
}

} // namespace backtest