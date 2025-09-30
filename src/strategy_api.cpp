#include "strategy_api.h"
#include "strategy_engine.h"
#include <fstream>
#include <iostream>
#include <curl/curl.h>

using namespace std;

StrategyController::StrategyController() {
    loadDefaultConfig();
}

void StrategyController::setupRoutes(Rest::Router& router) {
    using namespace Rest;
    
    Routes::Post(router, "/backtest/run", Routes::bind(&StrategyController::runBacktest, this));
    Routes::Get(router, "/config", Routes::bind(&StrategyController::getConfig, this));
    Routes::Put(router, "/config", Routes::bind(&StrategyController::updateConfig, this));
    Routes::Get(router, "/health", Routes::bind(&StrategyController::getHealth, this));
    
    // CSV download endpoints
    Routes::Get(router, "/download/renko", Routes::bind(&StrategyController::downloadRenkoCSV, this));
    Routes::Get(router, "/download/trades", Routes::bind(&StrategyController::downloadTradesCSV, this));
    Routes::Get(router, "/download/summary", Routes::bind(&StrategyController::downloadSummaryCSV, this));
}

void StrategyController::runBacktest(const Rest::Request& request, Http::ResponseWriter response) {
    try {
        json request_body;
        StrategyConfig config = current_config_;
        
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
        response.headers().add<Http::Header::AccessControlAllowOrigin>("*");
        response.send(Http::Code::Ok, result.dump());
        
    } catch (const exception& e) {
        json error_response = {
            {"error", true},
            {"message", e.what()}
        };
        response.headers().add<Http::Header::AccessControlAllowOrigin>("*");
        response.send(Http::Code::Bad_Request, error_response.dump());
    }
}

void StrategyController::getConfig(const Rest::Request& request, Http::ResponseWriter response) {
    (void)request; // Mark as unused
    
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
    response.headers().add<Http::Header::AccessControlAllowOrigin>("*");
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
        response.headers().add<Http::Header::AccessControlAllowOrigin>("*");
        response.send(Http::Code::Ok, success_response.dump());
        
    } catch (const exception& e) {
        json error_response = {
            {"error", true},
            {"message", e.what()}
        };
        response.headers().add<Http::Header::AccessControlAllowOrigin>("*");
        response.send(Http::Code::Bad_Request, error_response.dump());
    }
}

void StrategyController::getHealth(const Rest::Request& request, Http::ResponseWriter response) {
    (void)request; // Mark as unused
    
    json health = {
        {"status", "healthy"},
        {"service", "Strategy Backtest API"},
        {"timestamp", time(nullptr)},
        {"version", "1.0.0"}
    };
    
    response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
    response.headers().add<Http::Header::AccessControlAllowOrigin>("*");
    response.send(Http::Code::Ok, health.dump());
}

void StrategyController::downloadRenkoCSV(const Rest::Request& request, Http::ResponseWriter response) {
    (void)request; // Mark as unused
    
    try {
        auto result = StrategyEngine::runBacktest(current_config_);
        string csv_content = StrategyEngine::generateCSV(result, "renko");
        
        // Set CSV headers - simplified without ContentDisposition
        response.headers().add<Http::Header::ContentType>("text/csv");
        response.send(Http::Code::Ok, csv_content);
        
    } catch (const exception& e) {
        response.send(Http::Code::Internal_Server_Error, e.what());
    }
}

void StrategyController::downloadTradesCSV(const Rest::Request& request, Http::ResponseWriter response) {
    (void)request; // Mark as unused
    
    try {
        auto result = StrategyEngine::runBacktest(current_config_);
        string csv_content = StrategyEngine::generateCSV(result, "trades");
        
        response.headers().add<Http::Header::ContentType>("text/csv");
        response.send(Http::Code::Ok, csv_content);
        
    } catch (const exception& e) {
        response.send(Http::Code::Internal_Server_Error, e.what());
    }
}

void StrategyController::downloadSummaryCSV(const Rest::Request& request, Http::ResponseWriter response) {
    (void)request; // Mark as unused
    
    try {
        auto result = StrategyEngine::runBacktest(current_config_);
        string csv_content = StrategyEngine::generateCSV(result, "summary");
        
        response.headers().add<Http::Header::ContentType>("text/csv");
        response.send(Http::Code::Ok, csv_content);
        
    } catch (const exception& e) {
        response.send(Http::Code::Internal_Server_Error, e.what());
    }
}

json StrategyController::runStrategyWithConfig(const StrategyConfig& config) {
    auto result = StrategyEngine::runBacktest(config);
    
    json response = {
        {"summary", result.summary},
        {"config_used", result.config_used},
        {"trades", result.trades},
        {"renko_data", result.renko_data},
        {"csv_downloads", {
            {"renko_data", "/download/renko"},
            {"trades_data", "/download/trades"}, 
            {"summary", "/download/summary"}
        }}
    };
    
    return response;
}

void StrategyController::loadDefaultConfig() {
    // Default values are already set in the struct definition
}

// StrategyAPI implementation
StrategyAPI::StrategyAPI(Address addr) 
    : httpEndpoint_(std::make_shared<Http::Endpoint>(addr)) {}

void StrategyAPI::init(size_t thr) {
    auto opts = Http::Endpoint::options()
        .threads(thr)
        .flags(Tcp::Options::ReuseAddr);
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
    strategyController_.setupRoutes(router_);
}