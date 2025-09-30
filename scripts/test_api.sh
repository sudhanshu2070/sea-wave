#!/bin/bash

echo "Testing Strategy Backtest API..."

# Health check
echo "1. Health check:"
curl -s http://localhost:9080/health | jq .

# Get config
echo -e "\n2. Current config:"
curl -s http://localhost:9080/config | jq .

# Run backtest with default config
echo -e "\n3. Running backtest with default config:"
curl -s -X POST http://localhost:9080/backtest/run | jq '.summary'

echo -e "\nAPI test completed!"