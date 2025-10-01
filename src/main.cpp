#include <pistache/endpoint.h>
#include <pistache/router.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <memory>
#include <curl/curl.h>
#include "../include/backtest/data_fetcher.hpp"
#include "../include/backtest/renko.hpp"
#include "../include/backtest/ichimoku.hpp"
#include "../include/backtest/strategy.hpp"
#include "../include/backtest/utils.hpp"

using namespace Pistache;
using json = nlohmann::json;
using namespace backtest;

class BacktestHandler {
public:
    void handleBacktest(const Rest::Request& request, Http::ResponseWriter response) {
        try {
            auto data = json::parse(request.body());

            // Parse config
            std::string symbol = data.value("symbol", "ETHUSDT");
            std::string resolution = data.value("resolution", "5m");
            double brick_size = data.value("brick_size", 40.0);
            double reversal_size = data.value("reversal_size", 80.0);
            std::string start_date = data.value("start_date", "2025-08-01");
            std::string start_time = data.value("start_time", "00:00:00");
            std::string end_date = data.value("end_date", "2025-09-01");
            std::string end_time = data.value("end_time", "23:59:59");
            int tenkan = data.value("tenkan", 5);
            int kijun = data.value("kijun", 26);
            int span_b = data.value("span_b", 52);

            auto start_ts = ist_to_unix(start_date, start_time);
            auto end_ts = ist_to_unix(end_date, end_time);

            auto candles = fetch_candles(symbol, resolution, start_ts, end_ts);
            auto renko = build_renko(candles, brick_size, reversal_size);
            auto [trades, logs] = run_strategy(renko, tenkan, kijun, span_b);

            // Recompute Ichimoku for CSV
            std::vector<double> closes;
            for (const auto& b : renko) closes.push_back(b.close);
            auto tenkan_line = donchian_mid(closes, tenkan);
            auto kijun_line = donchian_mid(closes, kijun);
            auto span_b_line = donchian_mid(closes, span_b);
            std::vector<double> span_a(closes.size());
            for (size_t i = 0; i < closes.size(); ++i) {
                if (!std::isnan(tenkan_line[i]) && !std::isnan(kijun_line[i]))
                    span_a[i] = (tenkan_line[i] + kijun_line[i]) / 2.0;
            }

            double net_profit = 0.0;
            for (const auto& t : trades) net_profit += t.profit;

            json result = {
                {"success", true},
                {"summary", {
                    {"renko_bricks", static_cast<int>(renko.size())},
                    {"trades", static_cast<int>(trades.size())},
                    {"net_profit", std::round(net_profit * 100.0) / 100.0}
                }},
                {"files", {
                    {"renko_with_ichimoku.csv", renko_to_csv(renko, tenkan_line, kijun_line, span_a, span_b_line)},
                    {"trades.csv", trades_to_csv(trades)},
                    {"strategy_logs.csv", logs_to_csv(logs)}
                }}
            };

            response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
            response.send(Http::Code::Ok, result.dump(2));

        } catch (const std::exception& e) {
            json err{{"error", std::string("Backtest failed: ") + e.what()}};
            response.send(Http::Code::Bad_Request, err.dump());
        }
    }
};

int main() {
    curl_global_init(CURL_GLOBAL_DEFAULT);

    Address addr(Ipv4::any(), Port(9080));
    auto opts = Http::Endpoint::options().threads(4);
    Http::Endpoint server(addr);
    server.init(opts);

    Rest::Router router;
    BacktestHandler handler;
    router.post("/backtest", Rest::Routes::bind(&BacktestHandler::handleBacktest, &handler));

    server.setHandler(router.handler());
    std::cout << "API running on http://localhost:9080/backtest\n";
    server.serve();

    curl_global_cleanup();
    return 0;
}