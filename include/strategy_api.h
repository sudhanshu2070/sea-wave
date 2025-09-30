#ifndef STRATEGY_API_H
#define STRATEGY_API_H

#include <pistache/endpoint.h>
#include <pistache/router.h>
#include <pistache/http.h>
#include <pistache/net.h>
#include <nlohmann/json.hpp>
#include <string>
#include <memory>

using namespace Pistache;
using json = nlohmann::json;

class StrategyController {
public:
    StrategyController() = default;
    
    void setupRoutes(Rest::Router& router);
    
    // API endpoints
    void runBacktest(const Rest::Request& request, Http::ResponseWriter response);
    void getConfig(const Rest::Request& request, Http::ResponseWriter response);
    void updateConfig(const Rest::Request& request, Http::ResponseWriter response);
    
private:
    // Default configuration
    struct StrategyConfig {
        std::string symbol = "ETHUSDT";
        std::string resolution = "5m";
        double brick_size = 40.0;
        double reversal_size = 80.0;
        std::string source_type = "ohlc4";
        std::string start_date = "2025-08-01";
        std::string start_time = "00:00:00";
        std::string end_date = "2025-09-01";
        std::string end_time = "23:59:59";
        
        // Ichimoku parameters
        int tenkan = 5;
        int kijun = 26;
        int span_b = 52;
        int displacement = 26;
    };
    
    StrategyConfig current_config_;
    
    json runStrategyWithConfig(const StrategyConfig& config);
};

class StrategyAPI {
public:
    StrategyAPI(Address addr);
    void init(size_t thr = 2);
    void start();
    void stop();
    
private:
    std::shared_ptr<Http::Endpoint> httpEndpoint_;
    Rest::Router router_;
};

#endif // STRATEGY_API_H