@echo off
REM Quick run script for Simple Media Server (Windows)

echo Starting Simple Media Server...
echo.

REM Try Visual Studio build first
if exist backend\build\Release\media_server.exe (
    echo Starting server (Release build)...
    cd backend\build\Release
    media_server.exe
    cd ..\..\..
    goto :end
)

REM Try MinGW build
if exist backend\build\media_server.exe (
    echo Starting server (MinGW build)...
    cd backend\build
    media_server.exe
    cd ..\..
    goto :end
)

REM Try Debug build
if exist backend\build\Debug\media_server.exe (
    echo Starting server (Debug build)...
    cd backend\build\Debug
    media_server.exe
    cd ..\..\..
    goto :end
)

echo Error: Server executable not found!
echo Please run build.bat first
pause
exit /b 1

:end
