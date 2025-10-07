#!/bin/bash

echo "Starting Backtest API Server..."

# Check if executable exists
if [ ! -f "./build/backtest_api" ]; then
    echo "Executable not found. Building first..."
    ./build.sh
fi

# Run the server
./build/backtest_api