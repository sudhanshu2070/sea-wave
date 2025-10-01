#include "../include/backtest/data_fetcher.hpp"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <thread>
#include <iostream>
#include <algorithm>
#include <sstream>  

using json = nlohmann::json;

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

static int resolution_seconds(const std::string& res) {
    std::string s = res;
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    if (s.size() > 1 && s.back() == 'm') {
        return std::stoi(s.substr(0, s.size()-1)) * 60;
    } else if (s.size() > 1 && s.back() == 'h') {
        return std::stoi(s.substr(0, s.size()-1)) * 3600;
    } else if (s.size() > 1 && s.back() == 'd') {
        return std::stoi(s.substr(0, s.size()-1)) * 86400;
    }
    return 60;
}

// MOVED OUTSIDE: Helper to map resolution to Binance interval
static std::string get_binance_interval(const std::string& res) {
    if (res == "1m") return "1m";
    if (res == "3m") return "3m";
    if (res == "5m") return "5m";
    if (res == "15m") return "15m";
    if (res == "30m") return "30m";
    if (res == "1h") return "1h";
    if (res == "2h") return "2h";
    if (res == "4h") return "4h";
    if (res == "6h") return "6h";
    if (res == "8h") return "8h";
    if (res == "12h") return "12h";
    if (res == "1d") return "1d";
    if (res == "3d") return "3d";
    if (res == "1w") return "1w";
    if (res == "1M") return "1M";
    return "5m";
}

namespace backtest {

std::vector<Candle> fetch_candles(
    const std::string& symbol,
    const std::string& resolution,
    long long start_time,
    long long end_time,
    int limit
) {
    int sec_per_candle = resolution_seconds(resolution);
    long long max_window_seconds = static_cast<long long>(limit) * sec_per_candle;
    std::vector<Candle> all_data;

    long long cur = start_time;
    while (cur <= end_time) {
        long long window_end = std::min(end_time, cur + max_window_seconds - 1);
        
        // ✅ Use the helper function
        std::string interval = get_binance_interval(resolution);
        std::ostringstream url;
        url << "https://api.binance.com/api/v3/klines"
            << "?symbol=" << symbol
            << "&interval=" << interval
            << "&startTime=" << (cur * 1000)
            << "&endTime=" << (window_end * 1000)
            << "&limit=" << limit;

        std::cout << "[fetch] " << cur << " -> " << window_end << "  URL: " << url.str() << std::endl;

        CURL* curl = curl_easy_init();
        std::string readBuffer;
        if (curl) {
            curl_easy_setopt(curl, CURLOPT_URL, url.str().c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            CURLcode res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);

            if (res != CURLE_OK) {
                throw std::runtime_error("CURL error: " + std::string(curl_easy_strerror(res)));
            }

            try {
                auto response = json::parse(readBuffer);

                if (!response.is_array()) {
                    throw std::runtime_error("Expected top-level JSON array from Binance");
                }

                if (response.empty()) {
                    cur = window_end + 1;
                    continue;
                }

                for (const auto& item : response) {
                    if (!item.is_array() || item.size() < 6) continue;

                    Candle c;
                    c.time = item[0].get<long long>() / 1000;
                    c.open = std::stod(item[1].get<std::string>());
                    c.high = std::stod(item[2].get<std::string>());
                    c.low = std::stod(item[3].get<std::string>());
                    c.close = std::stod(item[4].get<std::string>());
                    c.volume = std::stod(item[5].get<std::string>());
                    all_data.push_back(c);
                }

            } catch (const std::exception& e) {
                std::cerr << "Raw API response:\n" << readBuffer << "\n";
                throw std::runtime_error("JSON parse error: " + std::string(e.what()));
            }
        } else {
            throw std::runtime_error("Failed to init CURL");
        }

        cur = window_end + 1;
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    if (all_data.empty()) {
        throw std::runtime_error("No candles returned. Check symbol/times.");
    }

    std::sort(all_data.begin(), all_data.end(), [](const Candle& a, const Candle& b) {
        return a.time < b.time;
    });

    return all_data;
}

} // namespace backtest