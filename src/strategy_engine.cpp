#include "strategy_engine.h"
#include <iostream>
#include <curl/curl.h>
#include <sstream>
#include <iomanip>

using namespace std;

namespace StrategyEngine {

// Helper function to format dates
std::string formatDate(int day, int hour = 0) {
    stringstream ss;
    ss << "2025-08-" << setw(2) << setfill('0') << day 
       << "T" << setw(2) << setfill('0') << hour << ":00:00";
    return ss.str();
}

std::string formatDateTime(int day, const std::string& time) {
    stringstream ss;
    ss << "2025-08-" << setw(2) << setfill('0') << day << " " << time;
    return ss.str();
}

// Enhanced backtest with sample data for demonstration
BacktestResult runBacktest(const StrategyConfig& config) {
    BacktestResult result;
    
    // Sample trades data
    json trades = json::array();
    
    // Generate sample trades based on parameters
    for (int i = 0; i < 15; i++) {
        int day = (i % 15) + 1;
        double entry_price = 2500.0 + (i * 50.0);
        double exit_price = entry_price + (i % 2 == 0 ? 75.0 : -50.0);
        double profit = exit_price - entry_price;
        
        trades.push_back({
            {"entry_time", formatDateTime(day, "10:00:00")},
            {"entry_price", entry_price},
            {"exit_time", formatDateTime(day, "15:00:00")},
            {"exit_price", exit_price},
            {"direction", i % 2 == 0 ? "long" : "short"},
            {"profit", profit}
        });
    }
    
    // Generate sample renko data
    json renko_data = json::array();
    double price = 2400.0;
    for (int i = 0; i < 50; i++) {
        int day = (i % 30) + 1;
        int hour = i % 24;
        int dir = (i % 5 == 0) ? -1 : 1; // Every 5th brick is reversal
        double brick_open = price;
        double brick_close = price + (dir * config.brick_size);
        
        renko_data.push_back({
            {"brick_time", formatDate(day, hour)},
            {"open", brick_open},
            {"high", max(brick_open, brick_close)},
            {"low", min(brick_open, brick_close)},
            {"close", brick_close},
            {"dir", dir},
            {"reversal", (i % 5 == 0) ? "true" : "false"}
        });
        
        price = brick_close;
    }
    
    // Calculate performance metrics
    double total_profit = 0;
    int win = 0, loss = 0;
    
    for (auto& trade : trades) {
        double profit = trade["profit"];
        total_profit += profit;
        if (profit > 0) win++; else loss++;
    }
    
    double win_rate = trades.empty() ? 0 : (static_cast<double>(win) / trades.size()) * 100;
    
    result.summary = {
        {"total_trades", trades.size()},
        {"winning_trades", win},
        {"losing_trades", loss},
        {"win_rate", win_rate},
        {"total_profit", total_profit},
        {"max_drawdown", 125.50},
        {"avg_trade_profit", trades.empty() ? 0 : total_profit / trades.size()},
        {"profit_factor", 1.75},
        {"sharpe_ratio", 2.34}
    };
    
    result.config_used = {
        {"symbol", config.symbol},
        {"resolution", config.resolution},
        {"brick_size", config.brick_size},
        {"reversal_size", config.reversal_size},
        {"source_type", config.source_type},
        {"start_date", config.start_date},
        {"end_date", config.end_date},
        {"tenkan", config.tenkan},
        {"kijun", config.kijun},
        {"span_b", config.span_b},
        {"displacement", config.displacement}
    };
    
    result.trades = trades;
    result.renko_data = renko_data;
    
    cout << "Running backtest with config:" << endl;
    cout << "  Symbol: " << config.symbol << endl;
    cout << "  Brick Size: " << config.brick_size << endl;
    cout << "  Period: " << config.start_date << " to " << config.end_date << endl;
    cout << "  Generated " << trades.size() << " trades and " << renko_data.size() << " renko bricks" << endl;
    
    return result;
}

std::string generateCSV(const BacktestResult& result, const std::string& type) {
    stringstream ss;
    
    if (type == "summary") {
        ss << "Metric,Value" << endl;
        ss << "Total Trades," << result.summary["total_trades"] << endl;
        ss << "Winning Trades," << result.summary["winning_trades"] << endl;
        ss << "Losing Trades," << result.summary["losing_trades"] << endl;
        ss << "Win Rate %," << result.summary["win_rate"] << endl;
        ss << "Total Profit," << result.summary["total_profit"] << endl;
        ss << "Max Drawdown," << result.summary["max_drawdown"] << endl;
        ss << "Average Trade Profit," << result.summary["avg_trade_profit"] << endl;
        ss << "Profit Factor," << result.summary["profit_factor"] << endl;
        ss << "Sharpe Ratio," << result.summary["sharpe_ratio"] << endl;
        
    } else if (type == "trades") {
        ss << "Entry Time,Entry Price,Exit Time,Exit Price,Direction,Profit" << endl;
        for (const auto& trade : result.trades) {
            ss << trade["entry_time"].get<string>() << ","
               << trade["entry_price"].get<double>() << ","
               << trade["exit_time"].get<string>() << ","
               << trade["exit_price"].get<double>() << ","
               << trade["direction"].get<string>() << ","
               << trade["profit"].get<double>() << endl;
        }
        
    } else if (type == "renko") {
        ss << "Brick Time,Open,High,Low,Close,Direction,Reversal" << endl;
        for (const auto& brick : result.renko_data) {
            ss << brick["brick_time"].get<string>() << ","
               << brick["open"].get<double>() << ","
               << brick["high"].get<double>() << ","
               << brick["low"].get<double>() << ","
               << brick["close"].get<double>() << ","
               << brick["dir"].get<int>() << ","
               << brick["reversal"].get<string>() << endl;
        }
    }
    
    return ss.str();
}

} 