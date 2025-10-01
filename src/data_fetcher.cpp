#include "../include/backtest/data_fetcher.hpp"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <thread>
#include <iostream>
#include <algorithm>

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
        std::ostringstream url;
        url << "https://api.delta.exchange/v2/history/candles?symbol=" << symbol
            << "&resolution=" << resolution
            << "&start=" << cur
            << "&end=" << window_end
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
                auto result = response.value("result", json::array());
                if (result.empty()) {
                    cur = window_end + 1;
                    continue;
                }

                for (const auto& item : result) {
                    Candle c;
                    if (item.is_array()) {
                        c.time = item[0].get<long long>();
                        c.open = item[1].get<double>();
                        c.high = item[2].get<double>();
                        c.low = item[3].get<double>();
                        c.close = item[4].get<double>();
                        c.volume = item[5].get<double>();
                    } else {
                        c.time = item.value("time", 0LL);
                        c.open = item.value("open", 0.0);
                        c.high = item.value("high", 0.0);
                        c.low = item.value("low", 0.0);
                        c.close = item.value("close", 0.0);
                        c.volume = item.value("volume", 0.0);
                    }
                    all_data.push_back(c);
                }
            } catch (const std::exception& e) {
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