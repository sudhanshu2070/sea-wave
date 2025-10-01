#include "../include/backtest/utils.hpp"
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <chrono>
#include <ctime>

namespace backtest {

long long ist_to_unix(const std::string& date_str, const std::string& time_str) {
    std::string dt = date_str + " " + time_str;
    std::tm tm = {};
    std::istringstream ss(dt);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    if (ss.fail()) {
        throw std::runtime_error("Invalid date/time format: " + dt);
    }
    // Assume input is IST (UTC+5:30), convert to UTC
    std::time_t utc_time = std::mktime(&tm);
    utc_time -= 5 * 3600 + 30 * 60;
    return static_cast<long long>(utc_time);
}

static std::string double_to_string(double v, int precision = 2) {
    if (std::isnan(v)) return "";
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precision) << v;
    return oss.str();
}

std::string renko_to_csv(
    const std::vector<RenkoBrick>& bricks,
    const std::vector<double>& tenkan,
    const std::vector<double>& kijun,
    const std::vector<double>& span_a,
    const std::vector<double>& span_b
) {
    std::ostringstream ss;
    ss << "brick_time,brick_start_time,src_open,src_high,src_low,src_close,open,high,low,close,dir,reversal,tenkan,kijun,span_a,span_b\n";
    for (size_t i = 0; i < bricks.size(); ++i) {
        const auto& b = bricks[i];
        ss << b.brick_time << ","
           << b.brick_start_time << ","
           << b.src_open << ","
           << b.src_high << ","
           << b.src_low << ","
           << b.src_close << ","
           << b.open << ","
           << b.high << ","
           << b.low << ","
           << b.close << ","
           << b.dir << ","
           << (b.reversal ? "true" : "false") << ","
           << double_to_string(tenkan[i]) << ","
           << double_to_string(kijun[i]) << ","
           << double_to_string(span_a[i]) << ","
           << double_to_string(span_b[i]) << "\n";
    }
    return ss.str();
}

std::string trades_to_csv(const std::vector<Trade>& trades) {
    std::ostringstream ss;
    ss << "entry_time,entry_price,exit_time,exit_price,direction,profit\n";
    for (const auto& t : trades) {
        ss << t.entry_time << ","
           << t.entry_price << ","
           << t.exit_time << ","
           << t.exit_price << ","
           << t.direction << ","
           << t.profit << "\n";
    }
    return ss.str();
}

std::string logs_to_csv(const std::vector<LogEntry>& logs) {
    std::ostringstream ss;
    ss << "time,action,reason,close,kijun,cloud_top,cloud_bottom,long_entry_signal,long_exit_signal,short_entry_signal,short_exit_signal,position,available_renko\n";
    for (const auto& l : logs) {
        ss << l.time << ","
           << l.action << ","
           << "\"" << l.reason << "\","  // basic escaping
           << l.close << ","
           << double_to_string(l.kijun) << ","
           << double_to_string(l.cloud_top) << ","
           << double_to_string(l.cloud_bottom) << ","
           << (l.long_entry_signal ? "true" : "false") << ","
           << (l.long_exit_signal ? "true" : "false") << ","
           << (l.short_entry_signal ? "true" : "false") << ","
           << (l.short_exit_signal ? "true" : "false") << ","
           << l.position << ","
           << l.available_renko << "\n";
    }
    return ss.str();
}

} // namespace backtest