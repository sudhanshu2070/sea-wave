#include "../include/backtest/strategy.hpp"
#include "../include/backtest/ichimoku.hpp"
#include <cmath>
#include <algorithm>
#include <memory>   

namespace backtest {

std::pair<std::vector<Trade>, std::vector<LogEntry>> run_strategy(
    const std::vector<RenkoBrick>& ri,
    int tenkan_len,
    int kijun_len,
    int span_b_len
) {
    std::vector<Trade> trades;
    std::vector<LogEntry> logs;
    struct Position { std::string side; long long entry_time; double entry_price; };
    std::unique_ptr<Position> position = nullptr;

    std::vector<double> closes;
    for (const auto& b : ri) closes.push_back(b.close);

    auto tenkan = donchian_mid(closes, tenkan_len);
    auto kijun = donchian_mid(closes, kijun_len);
    auto span_b = donchian_mid(closes, span_b_len);
    std::vector<double> span_a(closes.size());
    for (size_t i = 0; i < closes.size(); ++i) {
        if (!std::isnan(tenkan[i]) && !std::isnan(kijun[i])) {
            span_a[i] = (tenkan[i] + kijun[i]) / 2.0;
        } else {
            span_a[i] = NAN;
        }
    }

    for (size_t i = 0; i < ri.size(); ++i) {
        const auto& row = ri[i];
        double c = row.close;
        double kijun_val = kijun[i];
        double sa = span_a[i];
        double sb = span_b[i];
        int available = static_cast<int>(i + 1);

        bool missing_components = std::isnan(kijun_val) || std::isnan(sa) || std::isnan(sb);
        if (missing_components) {
            std::string reason = "Not enough history for Ichimoku -> ";
            if (std::isnan(kijun_val)) {
                reason += "kijun: need " + std::to_string(kijun_len) + ", have " + std::to_string(available);
            }
            if (std::isnan(sa) || std::isnan(sb)) {
                if (!reason.empty() && reason.back() != '>') reason += "; ";
                reason += "span_b (or span_a): need " + std::to_string(span_b_len) + ", have " + std::to_string(available);
            }
            logs.push_back(LogEntry{
                row.brick_time, "skip", reason,
                c, kijun_val, NAN, NAN,
                false, false, false, false,
                position ? position->side : "none",
                available
            });
            continue;
        }

        double cloud_top = std::max(sa, sb);
        double cloud_bottom = std::min(sa, sb);
        bool inside_cloud = (c > cloud_bottom) && (c < cloud_top);

        bool long_entry = (c > kijun_val) && (c > cloud_top) && (!inside_cloud);
        bool long_exit = (c < kijun_val) || (c < cloud_top);
        bool short_entry = (c < kijun_val) && (c < cloud_bottom) && (!inside_cloud);
        bool short_exit = (c > kijun_val) || (c > cloud_bottom);

        if (!position) {
            if (long_entry) {
                position = std::make_unique<Position>(Position{"long", row.brick_time, c});
                logs.push_back(LogEntry{
                    row.brick_time, "enter_long", "Renko close > kijun and > cloud_top",
                    c, kijun_val, cloud_top, cloud_bottom,
                    true, false, false, false, "long", available
                });
            } else if (short_entry) {
                position = std::make_unique<Position>(Position{"short", row.brick_time, c});
                logs.push_back(LogEntry{
                    row.brick_time, "enter_short", "Renko close < kijun and < cloud_bottom",
                    c, kijun_val, cloud_top, cloud_bottom,
                    false, false, true, false, "short", available
                });
            } else {
                std::string reason;
                if (inside_cloud) {
                    reason = "No entry: price inside cloud";
                } else if (c <= kijun_val || c <= cloud_top) {
                    reason = "No entry: price not above all required Ichimoku levels for long";
                } else if (c >= kijun_val || c >= cloud_bottom) {
                    reason = "No entry: price not below all required Ichimoku levels for short";
                } else {
                    reason = "No entry: conditions not met";
                }
                logs.push_back(LogEntry{
                    row.brick_time, "no_trade", reason,
                    c, kijun_val, cloud_top, cloud_bottom,
                    long_entry, false, short_entry, false,
                    "none", available
                });
            }
        } else {
            if (position->side == "long") {
                if (long_exit) {
                    double profit = c - position->entry_price;
                    trades.push_back({position->entry_time, row.brick_time, position->entry_price, c, "long", profit});
                    logs.push_back(LogEntry{
                        row.brick_time, "exit_long", "Renko closed below kijun or below cloud_top",
                        c, kijun_val, cloud_top, cloud_bottom,
                        false, true, false, false, "none", available
                    });
                    position.reset();
                } else {
                    logs.push_back(LogEntry{
                        row.brick_time, "hold_long", "Exit conditions not met",
                        c, kijun_val, cloud_top, cloud_bottom,
                        false, false, false, false, "long", available
                    });
                }
            } else { // short
                if (short_exit) {
                    double profit = position->entry_price - c;
                    trades.push_back({position->entry_time, row.brick_time, position->entry_price, c, "short", profit});
                    logs.push_back(LogEntry{
                        row.brick_time, "exit_short", "Renko closed above kijun or above cloud_bottom",
                        c, kijun_val, cloud_top, cloud_bottom,
                        false, false, false, true, "none", available
                    });
                    position.reset();
                } else {
                    logs.push_back(LogEntry{
                        row.brick_time, "hold_short", "Exit conditions not met",
                        c, kijun_val, cloud_top, cloud_bottom,
                        false, false, false, false, "short", available
                    });
                }
            }
        }
    }

    // Force close at end
    if (position) {
        const auto& last = ri.back();
        double c = last.close;
        double profit = (position->side == "long") ? (c - position->entry_price) : (position->entry_price - c);
        trades.push_back({position->entry_time, last.brick_time, position->entry_price, c, position->side, profit});

        double cloud_top = std::max(span_a.back(), span_b.back());
        double cloud_bottom = std::min(span_a.back(), span_b.back());
        logs.push_back(LogEntry{
            last.brick_time, "force_exit_" + position->side, "Closed at end of backtest",
            c, kijun.back(), cloud_top, cloud_bottom,
            false, false, false, false, "none", static_cast<int>(ri.size())
        });
    }

    return {trades, logs};
}

} // namespace backtest