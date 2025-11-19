#!/bin/bash

# Build script for Linux/macOS

set -e

echo "Building Sokol Media Player..."

# Create build directory
mkdir -p build
cd build

# Configure
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build . -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

echo ""
echo "Build complete!"
echo "Run with: ./build/sokol_media_player"
