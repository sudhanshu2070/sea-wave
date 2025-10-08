#include <pistache/endpoint.h>
#include <pistache/router.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cmath>
#include <thread>
#include <chrono>
#include <curl/curl.h>

using json = nlohmann::json;
using namespace std;

// ======================
// COPY EXACTLY FROM strategyinc.cpp (START)
// ======================

// -------- STRUCTS --------
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
    string direction;
    double profit;
};

// -------- UTILITY FUNCTIONS --------
int64_t ist_to_unix(const string &date_str, const string &with_time = "00:00:00"){
    tm tm{};
    string dt = date_str + " " + with_time;
    istringstream ss(dt);
    ss >> get_time(&tm, "%Y-%m-%d %H:%M:%S");
#if defined(_WIN32)
    time_t epoch = _mkgmtime(&tm);
#else
    time_t epoch = timegm(&tm);
#endif
    const int IST_OFFSET = 5*3600 + 30*60;
    return static_cast<int64_t>(epoch) - IST_OFFSET;
}

int resolution_seconds(const string &res){
    string s = res; for(auto &c:s) c = tolower(c);
    if(!s.empty() && s.back()=='m') return stoi(s.substr(0,s.size()-1))*60;
    if(!s.empty() && s.back()=='h') return stoi(s.substr(0,s.size()-1))*3600;
    if(!s.empty() && s.back()=='d') return stoi(s.substr(0,s.size()-1))*86400;
    return 60;
}

size_t write_callback(void* contents, size_t size, size_t nmemb, string* s){
    size_t total = size*nmemb;
    s->append((char*)contents,total);
    return total;
}

string epoch_to_ist_iso(int64_t epoch_sec){
    const int IST_OFFSET = 5*3600+30*60;
    time_t t = static_cast<time_t>(epoch_sec+IST_OFFSET);
    tm tm{};
#if defined(_WIN32)
    gmtime_s(&tm,&t);
#else
    gmtime_r(&t,&tm);
#endif
    char buf[64];
    strftime(buf,sizeof(buf),"%Y-%m-%dT%H:%M:%S",&tm);
    return string(buf);
}

double get_source_price(const Candle &c, const string &source){
    string s = source; for(auto &ch:s) ch = tolower(ch);
    if(s=="close") return c.close;
    if(s=="open") return c.open;
    if(s=="high") return c.high;
    if(s=="low") return c.low;
    if(s=="hl2") return (c.high+c.low)/2.0;
    if(s=="hlc3") return (c.high+c.low+c.close)/3.0;
    if(s=="ohlc4") return (c.open+c.high+c.low+c.close)/4.0;
    throw runtime_error("Unsupported source type: "+source);
}

vector<Candle> fetch_candles(const string &symbol,const string &resolution,int64_t start_time,int64_t end_time,int limit=4000){
    int sec_per_candle = resolution_seconds(resolution);
    int64_t max_window_seconds = static_cast<int64_t>(limit)*sec_per_candle;
    vector<pair<int64_t,int64_t>> windows;
    int64_t cur = start_time;
    while(cur<=end_time){
        int64_t window_end = min(end_time, cur+max_window_seconds-1);
        windows.emplace_back(cur,window_end);
        cur = window_end+1;
    }
    vector<Candle> all_data;
    for(auto &w:windows){
        int64_t wstart=w.first, wend=w.second;
        // Fixed: remove extra spaces in URL
        string url = "https://api.delta.exchange/v2/history/candles?symbol="+symbol+"&resolution="+resolution+
                     "&start="+to_string(wstart)+"&end="+to_string(wend)+"&limit="+to_string(limit);
        string resp;
        CURL* curl = curl_easy_init();
        if(curl){
            curl_easy_setopt(curl,CURLOPT_URL,url.c_str());
            curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,write_callback);
            curl_easy_setopt(curl,CURLOPT_WRITEDATA,&resp);
            curl_easy_setopt(curl,CURLOPT_TIMEOUT,30L);
            struct curl_slist* headers=NULL;
            headers = curl_slist_append(headers,"accept: application/json");
            headers = curl_slist_append(headers,"User-Agent: cpp-client/1.0");
            curl_easy_setopt(curl,CURLOPT_HTTPHEADER,headers);
            CURLcode res = curl_easy_perform(curl);
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            if(res!=CURLE_OK) throw runtime_error(string("curl error: ")+curl_easy_strerror(res));
        }
        json j = json::parse(resp);
        if(!j.contains("result")) continue;
        json arr = j["result"];
        if(arr.is_null()||arr.empty()) continue;
        for(auto &item:arr){
            if(item.is_array() && item.size() >= 5) {
                // Helper lambda to safely get double (0.0 if null or missing)
                auto safe_get_double = [](const json& j, double fallback = 0.0) -> double {
                    if (j.is_number()) return j.get<double>();
                    if (j.is_null()) return fallback;
                    try { return j.get<double>(); }
                    catch(...) { return fallback; }
                    return fallback;
                };

                int64_t time = item[0].get<int64_t>();
                double open = safe_get_double(item[1]);
                double high = safe_get_double(item[2]);
                double low = safe_get_double(item[3]);
                double close = safe_get_double(item[4]);
                double volume = (item.size() > 5) ? safe_get_double(item[5]) : 0.0;

                // Skip invalid candles (e.g., all zeros)
                if (open == 0 && high == 0 && low == 0 && close == 0) {
                    continue;
                }

                all_data.push_back({time, open, high, low, close, volume});
            }
            else if(item.is_object()){
                // Same safe handling for object format
                auto get_or_zero = [](const json& j, const char* key) {
                    return j.contains(key) && !j[key].is_null() ? j[key].get<double>() : 0.0;
                };
                all_data.push_back({
                    item.value("time", 0),
                    get_or_zero(item, "open"),
                    get_or_zero(item, "high"),
                    get_or_zero(item, "low"),
                    get_or_zero(item, "close"),
                    get_or_zero(item, "volume")
                });
            }
        }
        this_thread::sleep_for(chrono::milliseconds(20));
    }
    if(all_data.empty()) throw runtime_error("No candles returned. Check symbol/times.");
    sort(all_data.begin(),all_data.end(),[](const Candle &a,const Candle &b){ return a.time<b.time; });
    return all_data;
}

vector<RenkoBrick> build_renko(const vector<Candle> &candles,double brick_size,double reversal_size,const string &source){
    vector<RenkoBrick> rows;
    double last_price=NAN;
    optional<int> last_dir;
    int64_t trend_start_time=0;
    for(auto &candle:candles){
        double price = get_source_price(candle,source);
        int64_t ts = candle.time;
        double src_o=candle.open,src_h=candle.high,src_l=candle.low,src_c=candle.close;
        if(isnan(last_price)){ last_price=price; trend_start_time=ts; continue;}
        bool progressed=true;
        while(progressed){
            progressed=false;
            if(!last_dir.has_value()){
                double diff = price-last_price;
                if(fabs(diff)>=brick_size){
                    int dir = diff>0?1:-1;
                    double new_close = last_price + dir*brick_size;
                    rows.push_back({ts, trend_start_time, src_o,src_h,src_l,src_c,last_price,max(last_price,new_close),min(last_price,new_close),new_close,dir,false});
                    last_price=new_close;
                    last_dir=dir;
                    progressed=true;
                }
            }else if(last_dir.value()==1){
                if(price>=last_price+brick_size){
                    double new_close=last_price+brick_size;
                    rows.push_back({ts, trend_start_time, src_o,src_h,src_l,src_c,last_price,new_close,last_price,new_close,1,false});
                    last_price=new_close; progressed=true;
                }else if(price<=last_price-reversal_size){
                    double new_close=last_price-reversal_size;
                    rows.push_back({ts, ts, src_o,src_h,src_l,src_c,last_price,last_price,new_close,new_close,-1,true});
                    last_price=new_close; last_dir=-1; trend_start_time=ts; progressed=true;
                }
            }else{
                if(price<=last_price-brick_size){
                    double new_close=last_price-brick_size;
                    rows.push_back({ts, trend_start_time, src_o,src_h,src_l,src_c,last_price,last_price,new_close,new_close,-1,false});
                    last_price=new_close; progressed=true;
                }else if(price>=last_price+reversal_size){
                    double new_close=last_price+reversal_size;
                    rows.push_back({ts, ts, src_o,src_h,src_l,src_c,last_price,new_close,last_price,new_close,1,true});
                    last_price=new_close; last_dir=1; trend_start_time=ts; progressed=true;
                }
            }
        }
    }
    return rows;
}

vector<double> donchian_mid(const vector<double> &series,int length){
    size_t n = series.size();
    vector<double> out(n,numeric_limits<double>::quiet_NaN());
    if(length<=0) return out;
    for(size_t i=length-1;i<n;i++){
        double hh=series[i-length+1]; double ll=series[i-length+1];
        for(size_t j=i-length+1;j<=i;j++){ hh=max(hh,series[j]); ll=min(ll,series[j]); }
        out[i]=(hh+ll)/2.0;
    }
    return out;
}

vector<IchimokuRow> ichimoku_on_renko(const vector<RenkoBrick> &renko,int tenkan_len=5,int kijun_len=26,int span_b_len=52,int displacement=26){
    size_t n = renko.size();
    vector<IchimokuRow> rows(n);
    vector<double> closes(n); for(size_t i=0;i<n;i++) closes[i]=renko[i].close;
    auto tenkan_mid=donchian_mid(closes,tenkan_len);
    auto kijun_mid=donchian_mid(closes,kijun_len);
    auto span_b_mid=donchian_mid(closes,span_b_len);
    for(size_t i=0;i<n;i++){
        rows[i].brick_time=renko[i].brick_time;
        rows[i].close=renko[i].close;
        rows[i].tenkan=tenkan_mid[i];
        rows[i].kijun=kijun_mid[i];
        rows[i].span_a = isnan(tenkan_mid[i])||isnan(kijun_mid[i])?NAN:(tenkan_mid[i]+kijun_mid[i])/2.0;
        rows[i].span_b = span_b_mid[i];
        rows[i].chikou = i>=displacement? closes[i-displacement]:NAN;
    }
    return rows;
}

vector<Trade> run_strategy(const vector<IchimokuRow> &ri){
    vector<Trade> trades;
    bool in_long=false, in_short=false;
    Trade current{};
    for(size_t i=1;i<ri.size();i++){
        double c = ri[i].close;
        double tenkan = ri[i].tenkan;
        double kijun = ri[i].kijun;
        double span_a = ri[i].span_a;
        double span_b = ri[i].span_b;
        double cloud_top = max(span_a,span_b);
        double cloud_bottom = min(span_a,span_b);
        bool inside_cloud = c > cloud_bottom && c < cloud_top;

        if(in_long && (c < kijun || c < cloud_top)){
            current.exit_time = ri[i].brick_time;
            current.exit_price = c;
            current.profit = current.exit_price - current.entry_price;
            trades.push_back(current);
            in_long=false;
        }
        if(in_short && (c > kijun || c > cloud_bottom)){
            current.exit_time = ri[i].brick_time;
            current.exit_price = c;
            current.profit = current.entry_price - current.exit_price;
            trades.push_back(current);
            in_short=false;
        }

        if(!in_long && !inside_cloud && c > kijun && c > cloud_top){
            current = {ri[i].brick_time, c, 0, 0, "long", 0};
            in_long=true;
        }
        if(!in_short && !inside_cloud && c < kijun && c < cloud_bottom){
            current = {ri[i].brick_time, c, 0, 0, "short", 0};
            in_short=true;
        }
    }
    if(in_long){
        current.exit_time = ri.back().brick_time;
        current.exit_price = ri.back().close;
        current.profit = current.exit_price - current.entry_price;
        trades.push_back(current);
    }
    if(in_short){
        current.exit_time = ri.back().brick_time;
        current.exit_price = ri.back().close;
        current.profit = current.entry_price - current.exit_price;
        trades.push_back(current);
    }
    return trades;
}

// -------- IN-MEMORY CSV GENERATORS (instead of file writes) --------
string renko_to_csv_string(const vector<RenkoBrick> &rows){
    ostringstream oss;
    oss << "brick_time,open,high,low,close,dir,reversal\n";
    for(auto &r:rows){
        oss << epoch_to_ist_iso(r.brick_time) << ","
            << r.open << ","
            << r.high << ","
            << r.low << ","
            << r.close << ","
            << r.dir << ","
            << (r.reversal ? "true" : "false") << "\n";
    }
    return oss.str();
}

string trades_to_csv_string(const vector<Trade> &trades){
    ostringstream oss;
    oss << "entry_time,entry_price,exit_time,exit_price,direction,profit\n";
    for(auto &t:trades){
        oss << epoch_to_ist_iso(t.entry_time) << ","
            << t.entry_price << ","
            << epoch_to_ist_iso(t.exit_time) << ","
            << t.exit_price << ","
            << t.direction << ","
            << t.profit << "\n";
    }
    return oss.str();
}

string summary_to_csv_string(const vector<Trade> &trades){
    ostringstream oss;
    oss << "total_trades,total_profit,winning_trades,losing_trades,max_drawdown\n";
    if(trades.empty()){
        oss << "0,0,0,0,0\n";
        return oss.str();
    }
    double total_profit=0;
    int win=0, loss=0;
    double max_drawdown=0, peak=0;
    vector<double> equity_curve;
    for(auto &t:trades){
        total_profit += t.profit;
        if(t.profit>0) win++; else loss++;
        double eq = (equity_curve.empty()?0:equity_curve.back()) + t.profit;
        equity_curve.push_back(eq);
        if(eq>peak) peak=eq;
        max_drawdown = max(max_drawdown, peak - eq);
    }
    oss << trades.size() << "," << total_profit << "," << win << "," << loss << "," << max_drawdown << "\n";
    return oss.str();
}

// ======================
// COPY FROM strategyinc.cpp (END)
// ======================

using namespace Pistache;

class BacktestHandler {
public:
    void handleBacktest(const Rest::Request& request, Http::ResponseWriter response) {
        auto start_time = chrono::steady_clock::now();
        string client_ip = request.address().host();
        cout << " BACKTEST REQUEST RECEIVED from " << client_ip << " at " << get_current_time() << endl;
        
        try {
            auto data = json::parse(request.body());

            // Log request parameters
            cout << "Request parameters:" << endl;
            cout << "   Symbol: " << data.value("symbol", "ETHUSDT") << endl;
            cout << "   Resolution: " << data.value("resolution", "5m") << endl;
            cout << "   Date range: " << data.value("start_date", "2023-08-01") << " to " 
                 << data.value("end_date", "2023-08-02") << endl;

            // Extract parameters
            string symbol = data.value("symbol", "ETHUSDT");
            string resolution = data.value("resolution", "5m");
            double brick_size = data.value("brick_size", 40.0);
            double reversal_size = data.value("reversal_size", 80.0);
            string source_type = data.value("source_type", "ohlc4");
            string start_date = data.value("start_date", "2023-08-01");
            string start_time_str = data.value("start_time", "00:00:00");
            string end_date = data.value("end_date", "2023-08-02");
            string end_time_str = data.value("end_time", "23:59:59");
            int tenkan = data.value("tenkan", 5);
            int kijun = data.value("kijun", 26);
            int span_b = data.value("span_b", 52);

            auto start_ts = ist_to_unix(start_date, start_time_str);
            auto end_ts = ist_to_unix(end_date, end_time_str);

            if (start_ts >= end_ts) {
                throw runtime_error("Start time must be before end time");
            }

            cout << "Fetching candles..." << endl;
            auto df = fetch_candles(symbol, resolution, start_ts, end_ts);
            cout << " Fetched " << df.size() << " candles" << endl;

            cout << "Building Renko bricks..." << endl;
            auto renko = build_renko(df, brick_size, reversal_size, source_type);
            cout << "Built " << renko.size() << " Renko bricks" << endl;

            cout << "Calculating Ichimoku..." << endl;
            auto ri = ichimoku_on_renko(renko, tenkan, kijun, span_b);
            cout << "Calculated Ichimoku for " << ri.size() << " bricks" << endl;

            cout << "Running strategy..." << endl;
            auto trades = run_strategy(ri);
            cout << "Strategy generated " << trades.size() << " trades" << endl;

            // Generate CSVs as strings
            string renko_csv = renko_to_csv_string(renko);
            string trades_csv = trades_to_csv_string(trades);
            string summary_csv = summary_to_csv_string(trades);

            double net_profit = 0.0;
            for (const auto& t : trades) net_profit += t.profit;
            net_profit = round(net_profit * 100.0) / 100.0;

            auto end_time = chrono::steady_clock::now();
            auto duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);

            // Return all CSV data in JSON response
            json result = {
                {"success", true},
                {"summary", {
                    {"renko_bricks", (int)renko.size()},
                    {"trades", (int)trades.size()},
                    {"net_profit", net_profit},
                    {"processing_time_ms", duration.count()}
                }},
                {"csv_files", {
                    {"renko_data", {
                        {"filename", "renko_with_ichimoku.csv"},
                        {"content", renko_csv}
                    }},
                    {"trades_data", {
                        {"filename", "trades.csv"}, 
                        {"content", trades_csv}
                    }},
                    {"summary_data", {
                        {"filename", "backtest_summary.csv"},
                        {"content", summary_csv}
                    }}
                }}
            };

            cout << "Backtest completed in " << duration.count() << "ms - " 
                 << trades.size() << " trades, Net Profit: " << net_profit << endl;

            response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
            response.send(Http::Code::Ok, result.dump(2));

        } catch (const exception& e) {
            auto end_time = chrono::steady_clock::now();
            auto duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);
            
            cout << "Backtest failed after " << duration.count() << "ms: " << e.what() << endl;
            
            json err{{"success", false}, {"error", string("Backtest failed: ") + e.what()}};
            response.send(Http::Code::Bad_Request, err.dump(2));
        }
    }

    // Simple CSV download endpoint - returns JSON with file content
    void handleDownloadCSV(const Rest::Request& request, Http::ResponseWriter response) {
        auto start_time = chrono::steady_clock::now();
        string client_ip = request.address().host();
        cout << " CSV DOWNLOAD REQUEST from " << client_ip << " at " << get_current_time() << endl;
        
        try {
            auto data = json::parse(request.body());

            // Extract parameters
            string symbol = data.value("symbol", "ETHUSDT");
            string resolution = data.value("resolution", "5m");
            double brick_size = data.value("brick_size", 40.0);
            double reversal_size = data.value("reversal_size", 80.0);
            string source_type = data.value("source_type", "ohlc4");
            string start_date = data.value("start_date", "2023-08-01");
            string start_time_str = data.value("start_time", "00:00:00");
            string end_date = data.value("end_date", "2023-08-02");
            string end_time_str = data.value("end_time", "23:59:59");
            int tenkan = data.value("tenkan", 5);
            int kijun = data.value("kijun", 26);
            int span_b = data.value("span_b", 52);
            string file_type = data.value("file_type", "all"); // renko, trades, summary, all

            auto start_ts = ist_to_unix(start_date, start_time_str);
            auto end_ts = ist_to_unix(end_date, end_time_str);

            if (start_ts >= end_ts) {
                throw runtime_error("Start time must be before end time");
            }

            cout << "Generating CSV data for " << file_type << "..." << endl;
            auto df = fetch_candles(symbol, resolution, start_ts, end_ts);
            auto renko = build_renko(df, brick_size, reversal_size, source_type);
            auto ri = ichimoku_on_renko(renko, tenkan, kijun, span_b);
            auto trades = run_strategy(ri);

            json result = {
                {"success", true},
                {"symbol", symbol},
                {"start_date", start_date},
                {"end_date", end_date}
            };

            if (file_type == "renko" || file_type == "all") {
                result["renko_data"] = {
                    {"filename", "renko_" + symbol + "_" + start_date + "_to_" + end_date + ".csv"},
                    {"content", renko_to_csv_string(renko)}
                };
            }

            if (file_type == "trades" || file_type == "all") {
                result["trades_data"] = {
                    {"filename", "trades_" + symbol + "_" + start_date + "_to_" + end_date + ".csv"},
                    {"content", trades_to_csv_string(trades)}
                };
            }

            if (file_type == "summary" || file_type == "all") {
                result["summary_data"] = {
                    {"filename", "summary_" + symbol + "_" + start_date + "_to_" + end_date + ".csv"},
                    {"content", summary_to_csv_string(trades)}
                };
            }

            response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
            response.send(Http::Code::Ok, result.dump(2));

            cout << "CSV data sent for " << file_type << " - " << renko.size() << " bricks, " << trades.size() << " trades" << endl;

        } catch (const exception& e) {
            cout << "CSV download failed: " << e.what() << endl;
            json err{{"success", false}, {"error", string("CSV download failed: ") + e.what()}};
            response.send(Http::Code::Bad_Request, err.dump(2));
        }
    }

private:
    string get_current_time() {
        auto now = chrono::system_clock::now();
        auto in_time_t = chrono::system_clock::to_time_t(now);
        
        char buf[100];
        tm tm_buf;
        localtime_r(&in_time_t, &tm_buf);
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_buf);
        return string(buf);
    }
};

int main() {
    cout << "Initializing Backtest API Server (CSV Download Edition)..." << endl;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    cout << "cURL initialized" << endl;

    const int PORT = 9080;
    Address addr(Ipv4::any(), Port(PORT));
    auto opts = Http::Endpoint::options().threads(4);
    Http::Endpoint server(addr);
    server.init(opts);

    Rest::Router router;

    // Health endpoint
    Rest::Routes::Get(router, "/backtest/health",
        [](const Rest::Request&, Http::ResponseWriter response) -> Rest::Route::Result {
            nlohmann::json result = {{"status", "ok"}, {"message", "Server is running"}};
            response.send(Http::Code::Ok, result.dump(), MIME(Application, Json));
            return Rest::Route::Result::Ok;
        });

    // Backtest handler
    BacktestHandler handler;
    
    // Regular backtest (returns JSON with CSV data as strings)
    router.post("/backtest", Rest::Routes::bind(&BacktestHandler::handleBacktest, &handler));
    
    // CSV download endpoint (returns JSON with CSV content)
    router.post("/backtest/download-csv", Rest::Routes::bind(&BacktestHandler::handleDownloadCSV, &handler));

    server.setHandler(router.handler());

    cout << "==================================================" << endl;
    cout << "BACKTEST API SERVER STARTED!" << endl;
    cout << "Endpoints:" << endl;
    cout << "  POST /backtest              - Run backtest, returns JSON with CSV data" << endl;
    cout << "  POST /backtest/download-csv  - Get CSV data as JSON (specify file_type)" << endl;
    cout << "  GET  /backtest/health       - Health check" << endl;
    cout << "==================================================" << endl;
    cout << "Server running on port " << PORT << endl;
    cout << "Ready to accept requests..." << endl;

    server.serve();

    cout << "Server shutting down..." << endl;
    curl_global_cleanup();
    return 0;
}