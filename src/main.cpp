// main.cpp
// This is the entry point for the Pistache server.
// Compile with: g++ -std=c++17 -o trade_api main.cpp Renko.cpp Ichimoku.cpp Strategy.cpp BacktestController.cpp -lpistache -lcurl -lpthread
// Assumes Pistache, libcurl, and nlohmann/json are installed.
// nlohmann/json can be single-include: https://github.com/nlohmann/json

#include <pistache/endpoint.h>
#include <pistache/router.h>
#include "BacktestController.h"

int main(int argc, char* argv[]) {
    Pistache::Address addr(Pistache::Ipv4::any(), Pistache::Port(8080));
    auto opts = Pistache::Http::Endpoint::options().threads(4);
    Pistache::Http::Endpoint server(addr);
    server.init(opts);

    Pistache::Rest::Router router;
    BacktestController::setupRoutes(router);

    server.setHandler(router.handler());
    std::cout << "Server starting on port 8080" << std::endl;
    server.serve();
    return 0;
}