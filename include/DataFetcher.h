#ifndef DATA_FETCHER_H
#define DATA_FETCHER_H

#include <vector>
#include <algorithm>
#include <string>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <thread>
#include "OHLC.h"

using json = nlohmann::json;

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

int resolution_seconds(const std::string& res) {
    std::string s = res;
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    if (s.back() == 'm') return std::stoi(s.substr(0, s.size() - 1)) * 60;
    if (s.back() == 'h') return std::stoi(s.substr(0, s.size() - 1)) * 3600;
    if (s.back() == 'd') return std::stoi(s.substr(0, s.size() - 1)) * 86400;
    return 60; // Default to 1 minute
}

std::vector<OHLC> fetchCandles(const std::string& symbol, const std::string& resolution, long start_time, long end_time, int limit = 4000) {
    std::vector<OHLC> candles;
    std::string base_url = "https://api.delta.exchange/v2/history/candles";
    int sec_per_candle = resolution_seconds(resolution);
    long max_window_seconds = limit * sec_per_candle;
    std::vector<std::pair<long, long>> windows;
    long cur = start_time;
    while (cur <= end_time) {
        long window_end = std::min(end_time, cur + max_window_seconds - 1);
        windows.emplace_back(cur, window_end);
        cur = window_end + 1;
    }

    for (const auto& [wstart, wend] : windows) {
        std::string url = base_url + "?symbol=" + symbol + "&resolution=" + resolution +
                          "&start=" + std::to_string(wstart) + "&end=" + std::to_string(wend) +
                          "&limit=" + std::to_string(limit);
        std::cout << "[fetch] " << wstart << " -> " << wend << " URL: " << url << std::endl;

        CURL* curl = curl_easy_init();
        if (!curl) {
            std::cerr << "CURL initialization failed" << std::endl;
            continue;
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
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

        CURLcode res = curl_easy_perform(curl);
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        std::cout << "HTTP Response Code: " << http_code << std::endl;

        if (res != CURLE_OK) {
            std::cerr << "CURL error: " << curl_easy_strerror(res) << std::endl;
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            continue;
        }

        if (http_code != 200) {
            std::cerr << "HTTP error: " << http_code << " - Response: " << response_string.substr(0, 200) << "..." << std::endl;
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            continue;
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
            } else {
                std::cerr << "Invalid response format: missing or invalid 'result' array" << std::endl;
                if (data.contains("error")) {
                    std::cerr << "API error: " << data["error"].get<std::string>() << std::endl;
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "JSON parsing error: " << e.what() << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    if (candles.empty()) {
        std::cerr << "No candles returned. Check symbol/times." << std::endl;
        return candles;
    }

    std::sort(candles.begin(), candles.end(), [](const OHLC& a, const OHLC& b) {
        return a.time < b.time;
    });
    std::cout << "Fetched and sorted " << candles.size() << " candles (ascending by time)" << std::endl;
    return candles;
}

#endif