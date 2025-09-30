#!/bin/bash

set -e

echo "Building Strategy Backtest API..."

# Create build directory
mkdir -p build
cd build

# Configure and build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)

echo "Build completed successfully!"
echo "Binary location: ./bin/strategy_api"