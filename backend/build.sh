#!/bin/bash

# Build script for C++ backend

echo "Building backend..."

# Create build directory
mkdir -p build
cd build

# Configure with CMake
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build . -j$(nproc)

echo ""
echo "Backend built successfully!"
echo "Executable: backend/build/media_server"
