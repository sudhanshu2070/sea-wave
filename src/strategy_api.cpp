#include "strategy_api.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <ctime>
#include <thread>
#include <chrono>
#include <numeric>
#include <curl/curl.h>
#include "nlohmann/json.hpp"

// Include all the existing strategy code here (or link separately)
// For brevity, I'll show how to integrate the existing functions

using namespace std;

// -------- Forward declarations of existing functions --------
// (These should be in a separate header or the same file)

struct Candle {
    int64_t time;
    double open, high, low, close, volume;
};

struct RenkoBrick {
    int64_t brick_time, brick_start_time;
    double src_open, src_high, src_low, src_close;
    double open, high, low, close;
    int dir;
    bool reversal;
};

struct IchimokuRow {
    int64_t brick_time;
    double close;
    double tenkan, kijun, span_a, span_b, chikou;
};

struct Trade {
    int64_t entry_time;
    double entry_price;
    int64_t exit_time;
    double exit_price;
    std::string direction;
    double profit;
};

// Existing function declarations
int64_t ist_to_unix(const std::string &date_str, const std::string &with_time = "00:00:00");
int resolution_seconds(const std::string &res);
size_t write_callback(void* contents, size_t size, size_t nmemb, std::string* s);
std::string epoch_to_ist_iso(int64_t epoch_sec);
double get_source_price(const Candle &c, const std::string &source);
std::vector<Candle> fetch_candles(const std::string &symbol, const std::string &resolution, 
                                 int64_t start_time, int64_t end_time, int limit = 4000);
std::vector<RenkoBrick> build_renko(const std::vector<Candle> &candles, double brick_size, 
                                   double reversal_size, const std::string &source);
std::vector<IchimokuRow> ichimoku_on_renko(const std::vector<RenkoBrick> &renko, 
                                          int tenkan_len, int kijun_len, int span_b_len, int displacement);
std::vector<Trade> run_strategy(const std::vector<IchimokuRow> &ri);

// -------- StrategyController Implementation --------

void StrategyController::setupRoutes(Rest::Router& router) {
    using namespace Rest;
    
    Routes::Post(router, "/backtest/run", Routes::bind(&StrategyController::runBacktest, this));
    Routes::Get(router, "/config", Routes::bind(&StrategyController::getConfig, this));
    Routes::Put(router, "/config", Routes::bind(&StrategyController::updateConfig, this));
}

void StrategyController::runBacktest(const Rest::Request& request, Http::ResponseWriter response) {
    try {
        json request_body;
        StrategyConfig config = current_config_; // Start with current config
        
        // Parse request body if provided
        if (!request.body().empty()) {
            request_body = json::parse(request.body());
            
            // Update config with provided values
            if (request_body.contains("symbol")) 
                config.symbol = request_body["symbol"];
            if (request_body.contains("resolution")) 
                config.resolution = request_body["resolution"];
            if (request_body.contains("brick_size")) 
                config.brick_size = request_body["brick_size"];
            if (request_body.contains("reversal_size")) 
                config.reversal_size = request_body["reversal_size"];
            if (request_body.contains("source_type")) 
                config.source_type = request_body["source_type"];
            if (request_body.contains("start_date")) 
                config.start_date = request_body["start_date"];
            if (request_body.contains("start_time")) 
                config.start_time = request_body["start_time"];
            if (request_body.contains("end_date")) 
                config.end_date = request_body["end_date"];
            if (request_body.contains("end_time")) 
                config.end_time = request_body["end_time"];
            if (request_body.contains("tenkan")) 
                config.tenkan = request_body["tenkan"];
            if (request_body.contains("kijun")) 
                config.kijun = request_body["kijun"];
            if (request_body.contains("span_b")) 
                config.span_b = request_body["span_b"];
            if (request_body.contains("displacement")) 
                config.displacement = request_body["displacement"];
        }
        
        auto result = runStrategyWithConfig(config);
        
        response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
        response.send(Http::Code::Ok, result.dump());
        
    } catch (const std::exception& e) {
        json error_response = {
            {"error", true},
            {"message", e.what()}
        };
        response.send(Http::Code::Bad_Request, error_response.dump());
    }
}

void StrategyController::getConfig(const Rest::Request& request, Http::ResponseWriter response) {
    json config_json = {
        {"symbol", current_config_.symbol},
        {"resolution", current_config_.resolution},
        {"brick_size", current_config_.brick_size},
        {"reversal_size", current_config_.reversal_size},
        {"source_type", current_config_.source_type},
        {"start_date", current_config_.start_date},
        {"start_time", current_config_.start_time},
        {"end_date", current_config_.end_date},
        {"end_time", current_config_.end_time},
        {"tenkan", current_config_.tenkan},
        {"kijun", current_config_.kijun},
        {"span_b", current_config_.span_b},
        {"displacement", current_config_.displacement}
    };
    
    response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
    response.send(Http::Code::Ok, config_json.dump());
}

void StrategyController::updateConfig(const Rest::Request& request, Http::ResponseWriter response) {
    try {
        json request_body = json::parse(request.body());
        
        if (request_body.contains("symbol")) 
            current_config_.symbol = request_body["symbol"];
        if (request_body.contains("resolution")) 
            current_config_.resolution = request_body["resolution"];
        if (request_body.contains("brick_size")) 
            current_config_.brick_size = request_body["brick_size"];
        if (request_body.contains("reversal_size")) 
            current_config_.reversal_size = request_body["reversal_size"];
        if (request_body.contains("source_type")) 
            current_config_.source_type = request_body["source_type"];
        if (request_body.contains("start_date")) 
            current_config_.start_date = request_body["start_date"];
        if (request_body.contains("start_time")) 
            current_config_.start_time = request_body["start_time"];
        if (request_body.contains("end_date")) 
            current_config_.end_date = request_body["end_date"];
        if (request_body.contains("end_time")) 
            current_config_.end_time = request_body["end_time"];
        if (request_body.contains("tenkan")) 
            current_config_.tenkan = request_body["tenkan"];
        if (request_body.contains("kijun")) 
            current_config_.kijun = request_body["kijun"];
        if (request_body.contains("span_b")) 
            current_config_.span_b = request_body["span_b"];
        if (request_body.contains("displacement")) 
            current_config_.displacement = request_body["displacement"];
        
        json success_response = {
            {"success", true},
            {"message", "Configuration updated successfully"}
        };
        
        response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
        response.send(Http::Code::Ok, success_response.dump());
        
    } catch (const std::exception& e) {
        json error_response = {
            {"error", true},
            {"message", e.what()}
        };
        response.send(Http::Code::Bad_Request, error_response.dump());
    }
}

json StrategyController::runStrategyWithConfig(const StrategyConfig& config) {
    int64_t start_ts = ist_to_unix(config.start_date, config.start_time);
    int64_t end_ts   = ist_to_unix(config.end_date, config.end_time);
    
    auto df = fetch_candles(config.symbol, config.resolution, start_ts, end_ts);
    auto renko = build_renko(df, config.brick_size, config.reversal_size, config.source_type);
    auto ri = ichimoku_on_renko(renko, config.tenkan, config.kijun, config.span_b, config.displacement);
    auto trades = run_strategy(ri);
    
    // Calculate performance metrics
    double total_profit = 0;
    int win = 0, loss = 0;
    double max_drawdown = 0, peak = 0;
    vector<double> equity_curve;
    
    for (auto &t : trades) {
        total_profit += t.profit;
        if (t.profit > 0) win++; else loss++;
        double eq = (equity_curve.empty() ? 0 : equity_curve.back()) + t.profit;
        equity_curve.push_back(eq);
        if (eq > peak) peak = eq;
        max_drawdown = max(max_drawdown, peak - eq);
    }
    
    double win_rate = trades.empty() ? 0 : (static_cast<double>(win) / trades.size()) * 100;
    
    // Prepare JSON response
    json result = {
        {"summary", {
            {"total_trades", trades.size()},
            {"winning_trades", win},
            {"losing_trades", loss},
            {"win_rate", win_rate},
            {"total_profit", total_profit},
            {"max_drawdown", max_drawdown},
            {"avg_trade_profit", trades.empty() ? 0 : total_profit / trades.size()}
        }},
        {"config_used", {
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
        }}
    };
    
    // Add trades data
    json trades_json = json::array();
    for (const auto& trade : trades) {
        trades_json.push_back({
            {"entry_time", epoch_to_ist_iso(trade.entry_time)},
            {"entry_price", trade.entry_price},
            {"exit_time", epoch_to_ist_iso(trade.exit_time)},
            {"exit_price", trade.exit_price},
            {"direction", trade.direction},
            {"profit", trade.profit}
        });
    }
    result["trades"] = trades_json;
    
    return result;
}

// -------- StrategyAPI Implementation --------

StrategyAPI::StrategyAPI(Address addr) 
    : httpEndpoint_(std::make_shared<Http::Endpoint>(addr)) {}

void StrategyAPI::init(size_t thr) {
    auto opts = Http::Endpoint::options()
        .threads(thr)
        .flags(Tcp::Options::InstallSignalHandler);
    httpEndpoint_->init(opts);
    setupRoutes();
}

void StrategyAPI::start() {
    httpEndpoint_->setHandler(router_.handler());
    httpEndpoint_->serve();
}

void StrategyAPI::stop() {
    httpEndpoint_->shutdown();
}

void StrategyAPI::setupRoutes() {
    StrategyController strategyController;
    strategyController.setupRoutes(router_);
    
    // Health check endpoint
    Routes::Get(router_, "/health", Routes::bind([](const Rest::Request& request, Http::ResponseWriter response) {
        json health = {{"status", "healthy"}, {"service", "Strategy Backtest API"}};
        response.send(Http::Code::Ok, health.dump());
    }));
}