# Strategy Backtest API

A RESTful API for running Renko + Ichimoku strategy backtests with configurable parameters.

## Features

- RESTful API using Pistache
- Configurable Renko brick size and reversal
- Customizable Ichimoku parameters
- Real-time market data integration
- Comprehensive backtest results

## Build Instructions

```bash
# Clone and build
git clone <repository>
cd strategy-backtest-api
mkdir build && cd build
cmake ..
make

# Run the server
./bin/strategy_api