# Simple Media Server - Windows Setup

## 📋 Prerequisites

Install these before building:

1. **Node.js 18+** - https://nodejs.org
2. **CMake 3.14+** - https://cmake.org/download/
3. **C++ Compiler** - Choose one:
   - **Visual Studio 2019+** (Recommended) - https://visualstudio.microsoft.com/downloads/
   - **MinGW-w64** - https://www.mingw-w64.org/
4. **ffmpeg** (for video streaming) - https://ffmpeg.org/download.html
   - Add to PATH or place in backend/build folder

---

## 🚀 Quick Start

### **1. Build Everything**
```cmd
build.bat
```

This will:
- Install frontend dependencies
- Build Svelte frontend (optimized)
- Build C++ backend

### **2. Configure**
Edit `config.json`:
```json
{
  "libraryPath": "C:\\Users\\YourName\\Videos",
  "port": 8080,
  "profiles": [
    {"id": "user1", "name": "John", "icon": "👤"}
  ]
}
```

### **3. Run Server**
```cmd
run.bat
```

### **4. Open Browser**
```
http://localhost:8080
```

---

## 🛠️ Available Scripts

### **build.bat**
Complete build of frontend + backend
```cmd
build.bat
```

### **run.bat**
Start the server (after building)
```cmd
run.bat
```

### **dev.bat**
Development mode with hot-reload
```cmd
dev.bat
```
Opens frontend on http://localhost:5173 (requires backend running on :8080)

---

## 📁 Manual Build (Advanced)

### **Frontend Only**
```cmd
cd frontend-svelte
npm install
npm run build
cd ..
```

### **Backend Only**
```cmd
cd backend
mkdir build
cd build
cmake ..
cmake --build . --config Release
cd ..\..
```

### **Run Server**
```cmd
REM Visual Studio build:
cd backend\build\Release
media_server.exe

REM Or MinGW build:
cd backend\build
media_server.exe
```

---

## 🐛 Troubleshooting

### **CMake not found**
- Install CMake and add to PATH
- Or use "Add CMake to system PATH" during installation

### **Compiler not found**
- **Visual Studio**: Install "Desktop development with C++" workload
- **MinGW**: Ensure `g++` is in PATH

### **npm install fails**
- Try: `npm cache clean --force`
- Then: `npm install` again

### **Port already in use**
- Change port in `config.json`
- Or kill process using port 8080:
  ```cmd
  netstat -ano | findstr :8080
  taskkill /PID <process_id> /F
  ```

### **ffmpeg not found**
- Download from https://ffmpeg.org
- Extract and add `bin` folder to PATH
- Or place `ffmpeg.exe` in `backend\build` folder

---

## 📖 Project Structure

```
simple_media_server/
├── build.bat              # Main build script
├── run.bat                # Run server
├── dev.bat                # Development mode
├── config.json            # Server configuration
├── frontend-svelte/       # Svelte frontend
│   ├── src/               # Source code
│   └── dist/              # Built files (after npm run build)
└── backend/               # C++ backend
    ├── build/             # Build output
    └── main.cpp           # Server code
```

---

## 🎯 Development Workflow

1. **Start backend** (Terminal 1):
   ```cmd
   cd backend\build\Release
   media_server.exe
   ```

2. **Start frontend dev server** (Terminal 2):
   ```cmd
   dev.bat
   ```

3. **Edit code** - Changes auto-reload in browser!

4. **Build for production**:
   ```cmd
   build.bat
   ```

---

## 🔧 Visual Studio Users

If using Visual Studio, you can also:

1. Open `backend/build/media_server.sln` in Visual Studio
2. Build from IDE (F7)
3. Run with debugging (F5)

---

## 📝 Notes

- Default port: **8080**
- Frontend dev server: **5173**
- Logs appear in console
- HLS segments cached in temp directory
- Profile data stored in localStorage

---

## 🆘 Need Help?

- Check server logs in console
- Browser DevTools (F12) for frontend errors
- GitHub Issues: https://github.com/minervarr/simple_media_server/issues
