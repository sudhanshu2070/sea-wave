#include "strategy_api.h"
#include <iostream>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <curl/curl.h>

using namespace std;

std::unique_ptr<StrategyAPI> api;

void shutdownHandler(int signal) {
    cout << "\nShutting down server..." << endl;
    if (api) {
        api->stop();
    }
    exit(0);
}

void setupSignalHandlers() {
    signal(SIGINT, shutdownHandler);
    signal(SIGTERM, shutdownHandler);
}

int main() {
    try {
        // Initialize libcurl globally
        curl_global_init(CURL_GLOBAL_DEFAULT);
        
        setupSignalHandlers();
        
        Port port(9080);
        int thr = 4; // Number of threads
        
        Address addr(Ipv4::any(), port);
        cout << "==========================================" << endl;
        cout << "   Strategy Backtest API Server" << endl;
        cout << "   Port: " << port << endl;
        cout << "   Threads: " << thr << endl;
        cout << "==========================================" << endl;
        
        api = std::make_unique<StrategyAPI>(addr);
        api->init(thr);
        
        cout << "Server started successfully!" << endl;
        cout << "Available endpoints:" << endl;
        cout << "  POST /backtest/run" << endl;
        cout << "  GET  /config" << endl;
        cout << "  PUT  /config" << endl;
        cout << "  GET  /health" << endl;
        cout << "Press Ctrl+C to stop the server" << endl;
        
        api->start();
        
    } catch (const exception& e) {
        cerr << "Server error: " << e.what() << endl;
        curl_global_cleanup();
        return 1;
    }
    
    curl_global_cleanup();
    return 0;
}