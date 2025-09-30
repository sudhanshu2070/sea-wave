#!/bin/bash

# Change to script directory
cd "$(dirname "$0")"/..

# Check if binary exists
if [ ! -f "./bin/strategy_api" ]; then
    echo "Binary not found. Building first..."
    ./scripts/build.sh
fi

echo "Starting Strategy Backtest API..."
./bin/strategy_api