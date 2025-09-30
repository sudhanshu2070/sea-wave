#include "strategy_api.h"
#include <iostream>

using namespace std;

int main() {
    try {
        // Initialize libcurl globally
        curl_global_init(CURL_GLOBAL_DEFAULT);
        
        Port port(9080);
        int thr = 2;
        
        Address addr(Ipv4::any(), port);
        cout << "Starting Strategy Backtest API server on port " << port << " with " << thr << " threads..." << endl;
        
        StrategyAPI api(addr);
        api.init(thr);
        api.start();
        
    } catch (const exception& e) {
        cerr << "Server error: " << e.what() << endl;
        curl_global_cleanup();
        return 1;
    }
    
    curl_global_cleanup();
    return 0;
}