@echo off
REM Development mode - Frontend only (Windows)

echo Starting Svelte development server...
echo This will run the frontend with hot-reload on http://localhost:5173
echo API calls will proxy to backend on http://localhost:8080
echo.
echo Make sure backend is running separately!
echo.

cd frontend-svelte
call npm run dev

cd ..
