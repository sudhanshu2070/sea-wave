#ifndef STRATEGY_API_H
#define STRATEGY_API_H

#include "strategy_types.h"
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
    StrategyController();
    
    void setupRoutes(Rest::Router& router);
    
    // API endpoints
    void runBacktest(const Rest::Request& request, Http::ResponseWriter response);
    void getConfig(const Rest::Request& request, Http::ResponseWriter response);
    void updateConfig(const Rest::Request& request, Http::ResponseWriter response);
    void getHealth(const Rest::Request& request, Http::ResponseWriter response);
    
    // CSV download endpoints
    void downloadRenkoCSV(const Rest::Request& request, Http::ResponseWriter response);
    void downloadTradesCSV(const Rest::Request& request, Http::ResponseWriter response);
    void downloadSummaryCSV(const Rest::Request& request, Http::ResponseWriter response);
    
private:
    StrategyConfig current_config_;
    
    json runStrategyWithConfig(const StrategyConfig& config);
    void loadDefaultConfig();
    
    // CSV generation methods
    std::string generateRenkoCSV(const json& backtestResult);
    std::string generateTradesCSV(const json& backtestResult);
    std::string generateSummaryCSV(const json& backtestResult);
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
    StrategyController strategyController_;
    
    void setupRoutes();
};

#endif 