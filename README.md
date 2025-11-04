# Simple Media Server

A high-performance media server built with C++ and Rust WebAssembly for streaming your local video library to any device with native player support.

## Features

- **High Performance**: C++ backend with efficient video streaming
- **Smart Library Detection**: Automatically organizes TV series (Season/Episode patterns) and movies
- **Range Request Support**: Full seeking/scrubbing support in videos
- **Native Player Compatible**: Works with VLC, MPV, and other players via direct URLs
- **Modern WASM UI**: Rust + Yew compiled to WebAssembly for fast, responsive interface
- **AMOLED Black Theme**: Battery-friendly pure black design
- **Real-time Search**: Filter your library instantly
- **Mobile Optimized**: Long-press to share links with native video apps

## Architecture

```
Backend:  C++ with cpp-httplib (HTTP server + range requests)
Frontend: Rust + Yew → WebAssembly
Build:    CMake (C++) + Cargo + Trunk (Rust)
```

## Prerequisites

### Backend (C++)
- CMake 3.14+
- C++17 compatible compiler (GCC 8+, Clang 7+, MSVC 2019+)
- Internet connection (first build only, to download dependencies)

### Frontend (Rust)
- Rust 1.70+ (install from https://rustup.rs)
- Trunk (WASM build tool): `cargo install trunk`
- wasm32-unknown-unknown target: `rustup target add wasm32-unknown-unknown`

## Installation

### 1. Clone Repository

```bash
git clone https://github.com/yourusername/simple_media_server.git
cd simple_media_server
```

### 2. Configure Library Path

Edit `config.json`:

```json
{
  "library_path": "/path/to/your/videos",
  "host": "0.0.0.0",
  "port": 8080
}
```

**Important**: Use absolute path for `library_path`.

### 3. Build Frontend

```bash
cd frontend
./build.sh
```

This will:
- Install wasm32-unknown-unknown target if needed
- Compile Rust code to WebAssembly
- Generate optimized dist/ folder

### 4. Build Backend

```bash
cd ../backend
./build.sh
```

This will:
- Download cpp-httplib and nlohmann/json (first build only)
- Compile C++ server
- Create `backend/build/media_server` executable

## Usage

### Start the Server

```bash
cd backend/build
./media_server
```

Or with custom config:

```bash
./media_server /path/to/config.json
```

### Access the Web Interface

Open your browser:
```
http://localhost:8080
```

### Use with Native Players

**Desktop:**
- Right-click video link → Copy link → Paste in VLC/MPV

**Android:**
- Long-press video link → Share → Select VLC/MPV/MX Player
- Or use "Open with" browser feature

**iOS:**
- Tap video link → iOS will prompt to open in compatible app

## Library Organization

The server automatically detects and organizes your videos:

### TV Series Patterns

The scanner recognizes these patterns:

```
Series Name/
├── S01E01.mkv          # Standard: S01E01, S1E1
├── S01E02.mkv
├── Season 1/
│   ├── 1x01.mkv       # Alternative: 1x01
│   └── 1x02.mkv
└── Season 2/
    ├── Episode 1.mkv   # Named: Season X Episode Y
    └── Episode 2.mkv
```

### Movies

Any video file not matching series patterns is treated as a standalone movie.

### Supported Video Formats

- MP4, MKV, AVI, MOV, WMV, FLV, WebM
- M4V, MPG, MPEG, 3GP, OGV

## API Endpoints

### Get Library Structure
```
GET /api/library
```

Returns JSON with organized series and movies.

### Stream Video
```
GET /video/{relative_path}
```

Supports HTTP Range requests for seeking.

## Development

### Frontend Development

```bash
cd frontend
trunk serve
```

Runs dev server on http://localhost:8081 with hot reload.

### Backend Development

```bash
cd backend
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build .
./media_server
```

## Troubleshooting

### "library_path not set in config.json"
- Ensure config.json exists in project root
- Use absolute path, not relative

### "Failed to start server on port 8080"
- Port already in use, change `port` in config.json
- Check firewall settings

### Videos won't play on mobile
- Ensure your device is on the same network
- Use server's local IP (e.g., http://192.168.1.100:8080)
- Some formats may not be supported by mobile browsers

### Frontend not loading
- Ensure frontend was built: `cd frontend && ./build.sh`
- Check `frontend/dist/` directory exists
- Server must be run from `backend/build/` directory

## Security Notes

- **Local network only**: This server has no authentication
- **Do NOT expose to internet** without adding authentication
- For secure remote access, use VPN or SSH tunnel

## Performance

- **Concurrent streams**: Handles multiple simultaneous connections efficiently
- **Memory efficient**: Streams files directly, doesn't load into RAM
- **Fast startup**: Typically < 100ms
- **Low CPU usage**: I/O bound, minimal processing

## License

Apache License 2.0 - See LICENSE file

## Contributing

Pull requests welcome! Please ensure:
- C++ code follows C++17 standards
- Rust code passes `cargo clippy`
- Test on Linux/Windows/macOS if possible

## Roadmap

- [ ] User authentication
- [ ] Video thumbnails
- [ ] Resume playback tracking
- [ ] Subtitle support
- [ ] Multiple library paths
- [ ] Mobile app (Android/iOS)

## Credits

Built with:
- [cpp-httplib](https://github.com/yhirose/cpp-httplib) - HTTP server
- [nlohmann/json](https://github.com/nlohmann/json) - JSON parsing
- [Yew](https://yew.rs) - Rust WASM framework
