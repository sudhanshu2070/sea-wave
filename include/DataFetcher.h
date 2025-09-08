#ifndef DATA_FETCHER_H
#define DATA_FETCHER_H

#include <vector>
#include "OHLC.h"
#include <string>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <iostream>

using json = nlohmann::json;

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::vector<OHLC> fetchCandles(const std::string& symbol, const std::string& resolution, long start_time, long end_time) {
    std::vector<OHLC> candles;
    std::string base_url = "https://api.delta.exchange/v2/history/candles";
    std::string url = base_url + "?symbol=" + symbol + "&resolution=" + resolution + "&start=" + std::to_string(start_time) + "&end=" + std::to_string(end_time);

    std::cout << "Fetching URL: " << url << std::endl;

    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "CURL initialization failed" << std::endl;
        return candles;
    }

    std::string response_string;
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Accept: application/json");
    headers = curl_slist_append(headers, "User-Agent: TradeExecutorAPI/1.0 (curl)");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    // Disable SSL verification temporarily for debugging (remove in production)
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    std::cout << "HTTP Response Code: " << http_code << std::endl;
    if (res != CURLE_OK) {
        std::cerr << "CURL error: " << curl_easy_strerror(res) << std::endl;
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return candles;
    }

    std::cout << "Raw API Response: " << response_string << std::endl;

    if (http_code != 200) {
        std::cerr << "HTTP error: " << http_code << " - Response: " << response_string.substr(0, 200) << "..." << std::endl;
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return candles;
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    try {
        json data = json::parse(response_string);
        if (data.contains("result") && data["result"].is_array()) {
            for (auto& candle : data["result"]) {
                if (!candle.contains("time") || !candle.contains("open") || !candle.contains("high") ||
                    !candle.contains("low") || !candle.contains("close") || !candle.contains("volume")) {
                    std::cerr << "Invalid candle format: " << candle.dump() << std::endl;
                    continue;
                }
                OHLC ohlc;
                ohlc.time = candle["time"].get<long>();
                ohlc.open = candle["open"].get<double>();
                ohlc.high = candle["high"].get<double>();
                ohlc.low = candle["low"].get<double>();
                ohlc.close = candle["close"].get<double>();
                ohlc.volume = candle["volume"].get<double>();
                candles.push_back(ohlc);
            }
            std::cout << "Fetched " << candles.size() << " candles" << std::endl;
        } else {
            std::cerr << "Invalid response format: missing or invalid 'result' array" << std::endl;
            if (data.contains("error")) {
                std::cerr << "API error: " << data["error"].get<std::string>() << std::endl;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "JSON parsing error: " << e.what() << std::endl;
    }

    return candles;
}

#endif