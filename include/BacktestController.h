#ifndef BACKTEST_CONTROLLER_H
#define BACKTEST_CONTROLLER_H

#include <pistache/router.h>
#include <pistache/http.h>
#include "DataFetcher.h"
#include "Strategy.h"

class BacktestController {
public:
    static void setupRoutes(Pistache::Rest::Router& router) {
        Pistache::Rest::Routes::Get(router, "/backtest", Pistache::Rest::Routes::bind(&BacktestController::handleBacktest));
    }

    static void handleBacktest(const Pistache::Rest::Request& request, Pistache::Http::ResponseWriter response) {
        auto symbol = request.query().get("symbol").value_or("ETHUSDT");
        auto resolution = request.query().get("resolution").value_or("5m");
        long start = std::stol(request.query().get("start").value_or("1753996200")); // August 1, 2025, 00:00:00 IST
        long end = std::stol(request.query().get("end").value_or("1756715999"));     // September 1, 2025, 23:59:59 IST

        auto ohlc_data = fetchCandles(symbol, resolution, start, end);

        if (ohlc_data.empty()) {
            response.send(Pistache::Http::Code::Bad_Request, "Failed to fetch data");
            return;
        }

        Strategy strategy(40.0, 80.0, 5, 26, 52, 26);
        auto trades = strategy.backtest(ohlc_data);

        std::string result = "Backtest completed. Trades: " + std::to_string(trades.size()) + "\nCSVs exported: renko_with_ichimoku.csv, trades.csv, strategy_logs.csv";
        response.send(Pistache::Http::Code::Ok, result);
    }
};

#endif