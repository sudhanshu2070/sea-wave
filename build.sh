#!/bin/bash

echo "Building Backtest API..."

# Create build directory
mkdir -p build
cd build

# Run cmake and make
cmake ..
make

echo "Build completed! Executable: ./build/backtest_api"