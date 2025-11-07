#!/bin/bash

# Master build script for Simple Media Server

set -e  # Exit on error

echo "========================================="
echo "Simple Media Server - Build Script"
echo "========================================="
echo ""

# Check prerequisites
echo "Checking prerequisites..."

# Check CMake
if ! command -v cmake &> /dev/null; then
    echo "Error: CMake not found. Please install CMake 3.14+"
    exit 1
fi

# Check C++ compiler
if ! command -v g++ &> /dev/null && ! command -v clang++ &> /dev/null; then
    echo "Error: No C++ compiler found. Please install GCC or Clang"
    exit 1
fi

# Check Node.js
if ! command -v node &> /dev/null; then
    echo "Error: Node.js not found. Install from https://nodejs.org"
    exit 1
fi

# Check npm
if ! command -v npm &> /dev/null; then
    echo "Error: npm not found. Install Node.js which includes npm"
    exit 1
fi

echo "✓ All prerequisites found"
echo ""

# Build frontend
echo "========================================="
echo "Building Frontend (Svelte + TypeScript)"
echo "========================================="
cd frontend-svelte
npm install
npm run build
cd ..
echo ""

# Build backend
echo "========================================="
echo "Building Backend (C++)"
echo "========================================="
cd backend
./build.sh
cd ..
echo ""

echo "========================================="
echo "Build Complete!"
echo "========================================="
echo ""
echo "Next steps:"
echo "1. Edit config.json to set your video library path"
echo "2. Run: cd backend/build && ./media_server"
echo "3. Open: http://localhost:8080"
echo ""
