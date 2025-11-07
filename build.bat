@echo off
REM Master build script for Simple Media Server (Windows)

setlocal enabledelayedexpansion

echo =========================================
echo Simple Media Server - Build Script
echo =========================================
echo.

REM Check prerequisites
echo Checking prerequisites...

REM Check CMake
where cmake >nul 2>&1
if %errorlevel% neq 0 (
    echo Error: CMake not found. Please install CMake 3.14+
    echo Download from: https://cmake.org/download/
    exit /b 1
)

REM Check C++ compiler (Visual Studio or MinGW)
where cl >nul 2>&1
if %errorlevel% neq 0 (
    where g++ >nul 2>&1
    if !errorlevel! neq 0 (
        echo Error: No C++ compiler found.
        echo Please install Visual Studio or MinGW-w64
        exit /b 1
    )
)

REM Check Node.js
where node >nul 2>&1
if %errorlevel% neq 0 (
    echo Error: Node.js not found.
    echo Download from: https://nodejs.org
    exit /b 1
)

REM Check npm
where npm >nul 2>&1
if %errorlevel% neq 0 (
    echo Error: npm not found.
    echo Install Node.js which includes npm
    exit /b 1
)

echo √ All prerequisites found
echo.

REM Build frontend
echo =========================================
echo Building Frontend (Svelte + TypeScript)
echo =========================================
cd frontend-svelte
call npm install
if %errorlevel% neq 0 (
    echo Error: npm install failed
    cd ..
    exit /b 1
)

call npm run build
if %errorlevel% neq 0 (
    echo Error: Frontend build failed
    cd ..
    exit /b 1
)
cd ..
echo.

REM Build backend
echo =========================================
echo Building Backend (C++)
echo =========================================
cd backend

REM Create build directory if it doesn't exist
if not exist build mkdir build
cd build

REM Configure with CMake
cmake ..
if %errorlevel% neq 0 (
    echo Error: CMake configuration failed
    cd ..\..
    exit /b 1
)

REM Build
cmake --build . --config Release
if %errorlevel% neq 0 (
    echo Error: Backend build failed
    cd ..\..
    exit /b 1
)

cd ..\..
echo.

echo =========================================
echo Build Complete!
echo =========================================
echo.
echo Next steps:
echo 1. Edit config.json to set your video library path
echo 2. Run: cd backend\build\Release ^&^& media_server.exe
echo    (or: cd backend\build ^&^& media_server.exe for MinGW)
echo 3. Open: http://localhost:8080
echo.

pause
