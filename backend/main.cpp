#include <httplib.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <cstdlib>
#include <sstream>
#include <thread>
#include <mutex>
#include <map>
#include "scanner.h"

namespace fs = std::filesystem;
using json = nlohmann::json;

// HLS cache for generated segments
struct HLSCache {
    std::mutex mutex;
    std::map<std::string, std::string> playlists; // video_path -> m3u8 content
    std::map<std::string, fs::path> segmentDirs;  // video_path -> segment directory
};

// Generate HLS segments for a video file
bool generateHLS(const fs::path& videoPath, const fs::path& outputDir, std::string& playlistContent) {
    // Create output directory if it doesn't exist
    if (!fs::exists(outputDir)) {
        fs::create_directories(outputDir);
    }

    // Generate HLS segments using ffmpeg
    fs::path playlistPath = outputDir / "playlist.m3u8";
    fs::path segmentPattern = outputDir / "segment%d.ts";

    // Build ffmpeg command
    // -hls_time 4: 4 second segments for fast seeking
    // -hls_list_size 0: Include all segments in playlist
    // -hls_flags independent_segments: Enable fast seeking
    std::ostringstream cmd;
    cmd << "ffmpeg -i \"" << videoPath.string() << "\" "
        << "-codec: copy "  // Copy streams without re-encoding for speed
        << "-start_number 0 "
        << "-hls_time 4 "
        << "-hls_list_size 0 "
        << "-hls_flags independent_segments "
        << "-f hls "
        << "\"" << playlistPath.string() << "\" "
        << "2>&1";  // Redirect stderr to stdout

    std::cout << "Generating HLS for: " << videoPath << std::endl;
    int result = std::system(cmd.str().c_str());

    if (result != 0 || !fs::exists(playlistPath)) {
        std::cerr << "Failed to generate HLS segments" << std::endl;
        return false;
    }

    // Read generated playlist
    std::ifstream playlistFile(playlistPath);
    if (!playlistFile) {
        return false;
    }

    std::stringstream buffer;
    buffer << playlistFile.rdbuf();
    playlistContent = buffer.str();

    std::cout << "HLS generation complete: " << playlistPath << std::endl;
    return true;
}

// Profile structure
struct Profile {
    std::string id;
    std::string name;
    std::string icon;
};

// Configuration structure
struct Config {
    std::string libraryPath;
    int port = 8080;
    std::string host = "0.0.0.0";
    std::vector<Profile> profiles;

    static Config load(const std::string& configFile) {
        Config config;

        if (!fs::exists(configFile)) {
            std::cerr << "Config file not found: " << configFile << std::endl;
            std::cerr << "Using default configuration with default profile" << std::endl;
            // Add default profile if no config
            config.profiles.push_back({"default", "Default", "👤"});
            return config;
        }

        try {
            std::ifstream file(configFile);
            json j;
            file >> j;

            if (j.contains("library_path")) {
                config.libraryPath = j["library_path"].get<std::string>();
            }
            if (j.contains("port")) {
                config.port = j["port"].get<int>();
            }
            if (j.contains("host")) {
                config.host = j["host"].get<std::string>();
            }
            if (j.contains("profiles") && j["profiles"].is_array()) {
                auto profilesArray = j["profiles"];
                // Limit to max 5 profiles
                size_t maxProfiles = std::min(profilesArray.size(), size_t(5));
                for (size_t i = 0; i < maxProfiles; i++) {
                    auto& p = profilesArray[i];
                    if (p.contains("id") && p.contains("name")) {
                        Profile profile;
                        profile.id = p["id"].get<std::string>();
                        profile.name = p["name"].get<std::string>();
                        profile.icon = p.contains("icon") ? p["icon"].get<std::string>() : "👤";
                        config.profiles.push_back(profile);
                    }
                }
            }

            // If no profiles configured, add default
            if (config.profiles.empty()) {
                config.profiles.push_back({"default", "Default", "👤"});
            }
        } catch (const std::exception& e) {
            std::cerr << "Error parsing config: " << e.what() << std::endl;
            std::cerr << "Using default configuration" << std::endl;
            // Add default profile on error
            if (config.profiles.empty()) {
                config.profiles.push_back({"default", "Default", "👤"});
            }
        }

        return config;
    }
};

int main(int argc, char** argv) {
    // Load configuration
    std::string configPath = "../config.json";
    if (argc > 1) {
        configPath = argv[1];
    }

    Config config = Config::load(configPath);

    if (config.libraryPath.empty()) {
        std::cerr << "Error: library_path not set in config.json" << std::endl;
        std::cerr << "Please create config.json with: {\"library_path\": \"/path/to/videos\"}" << std::endl;
        return 1;
    }

    // Convert to absolute path
    fs::path libPath = fs::absolute(config.libraryPath);
    if (!fs::exists(libPath)) {
        std::cerr << "Error: Library path does not exist: " << libPath << std::endl;
        return 1;
    }

    std::cout << "Starting Simple Media Server..." << std::endl;
    std::cout << "Library path: " << libPath << std::endl;
    std::cout << "Scanning library..." << std::endl;

    // Scan library
    VideoScanner scanner(libPath.string());
    MediaLibrary library = scanner.scan();

    std::cout << "Found " << library.series.size() << " series and "
              << library.movies.size() << " movies" << std::endl;

    // Create HLS cache
    HLSCache hlsCache;

    // Create HLS cache directory
    fs::path hlsCacheDir = fs::temp_directory_path() / "media_server_hls";
    if (!fs::exists(hlsCacheDir)) {
        fs::create_directories(hlsCacheDir);
    }

    // Create HTTP server
    httplib::Server server;

    // Enable CORS for all routes
    server.set_default_headers({
        {"Access-Control-Allow-Origin", "*"},
        {"Access-Control-Allow-Methods", "GET, OPTIONS"},
        {"Access-Control-Allow-Headers", "Content-Type, Range"}
    });

    // Handle OPTIONS requests (CORS preflight)
    server.Options(".*", [](const httplib::Request&, httplib::Response& res) {
        res.status = 200;
    });

    // API endpoint: Get profiles
    server.Get("/api/profiles", [&config](const httplib::Request&, httplib::Response& res) {
        json profilesJson = json::array();
        for (const auto& profile : config.profiles) {
            profilesJson.push_back({
                {"id", profile.id},
                {"name", profile.name},
                {"icon", profile.icon}
            });
        }
        res.set_content(profilesJson.dump(), "application/json");
    });

    // API endpoint: Get library structure
    server.Get("/api/library", [&library](const httplib::Request&, httplib::Response& res) {
        json response = library.toJson();
        res.set_content(response.dump(), "application/json");
    });

    // Serve video files with range request support
    server.Get("/video/.*", [&libPath](const httplib::Request& req, httplib::Response& res) {
        // Extract video path from URL
        std::string videoPath = req.path.substr(7); // Remove "/video/"

        // Decode URL-encoded path
        videoPath = httplib::detail::decode_url(videoPath, false);

        // Security: prevent directory traversal
        if (videoPath.find("..") != std::string::npos) {
            res.status = 403;
            res.set_content("Forbidden", "text/plain");
            return;
        }

        fs::path fullPath = libPath / videoPath;

        if (!fs::exists(fullPath) || !fs::is_regular_file(fullPath)) {
            res.status = 404;
            res.set_content("Video not found", "text/plain");
            return;
        }

        // Determine content type based on extension
        std::string ext = fullPath.extension().string();
        std::string contentType = "video/mp4"; // Default

        if (ext == ".mkv") contentType = "video/x-matroska";
        else if (ext == ".webm") contentType = "video/webm";
        else if (ext == ".avi") contentType = "video/x-msvideo";
        else if (ext == ".mov") contentType = "video/quicktime";
        else if (ext == ".wmv") contentType = "video/x-ms-wmv";
        else if (ext == ".flv") contentType = "video/x-flv";
        else if (ext == ".m4v") contentType = "video/x-m4v";
        else if (ext == ".mpg" || ext == ".mpeg") contentType = "video/mpeg";
        else if (ext == ".3gp") contentType = "video/3gpp";
        else if (ext == ".ogv") contentType = "video/ogg";

        // Serve file with range request support (enables seeking)
        std::ifstream file(fullPath, std::ios::binary);
        if (!file) {
            res.status = 500;
            res.set_content("Error reading file", "text/plain");
            return;
        }

        // Get file size
        file.seekg(0, std::ios::end);
        size_t fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        // Check for Range header
        auto rangeHeader = req.get_header_value("Range");

        if (!rangeHeader.empty()) {
            // Parse range header (format: bytes=start-end)
            size_t start = 0, end = fileSize - 1;

            std::string rangeStr = rangeHeader;
            size_t pos = rangeStr.find("bytes=");
            if (pos != std::string::npos) {
                rangeStr = rangeStr.substr(pos + 6);
                size_t dashPos = rangeStr.find('-');

                if (dashPos != std::string::npos) {
                    std::string startStr = rangeStr.substr(0, dashPos);
                    std::string endStr = rangeStr.substr(dashPos + 1);

                    if (!startStr.empty()) start = std::stoull(startStr);
                    if (!endStr.empty()) end = std::stoull(endStr);
                }
            }

            // Validate range
            if (start >= fileSize) {
                res.status = 416; // Range Not Satisfiable
                return;
            }
            if (end >= fileSize) end = fileSize - 1;

            size_t contentLength = end - start + 1;

            // Set partial content response
            res.status = 206;
            res.set_header("Content-Range",
                "bytes " + std::to_string(start) + "-" +
                std::to_string(end) + "/" + std::to_string(fileSize));
            res.set_header("Content-Length", std::to_string(contentLength));
            res.set_header("Accept-Ranges", "bytes");
            res.set_header("Content-Type", contentType);

            // Read and send requested range
            file.seekg(start);
            std::vector<char> buffer(contentLength);
            file.read(buffer.data(), contentLength);
            res.set_content(buffer.data(), contentLength, contentType);

        } else {
            // Send entire file
            res.set_header("Accept-Ranges", "bytes");
            res.set_header("Content-Length", std::to_string(fileSize));

            std::vector<char> buffer(fileSize);
            file.read(buffer.data(), fileSize);
            res.set_content(buffer.data(), fileSize, contentType);
        }
    });

    // HLS playlist endpoint
    server.Get(R"(/hls/(.+)/playlist\.m3u8)", [&libPath, &hlsCache, &hlsCacheDir](const httplib::Request& req, httplib::Response& res) {
        std::string videoPath = httplib::detail::decode_url(req.matches[1].str(), false);

        // Security: prevent directory traversal
        if (videoPath.find("..") != std::string::npos) {
            res.status = 403;
            res.set_content("Forbidden", "text/plain");
            return;
        }

        fs::path fullPath = libPath / videoPath;

        if (!fs::exists(fullPath) || !fs::is_regular_file(fullPath)) {
            res.status = 404;
            res.set_content("Video not found", "text/plain");
            return;
        }

        // Check cache first
        std::lock_guard<std::mutex> lock(hlsCache.mutex);

        if (hlsCache.playlists.find(videoPath) == hlsCache.playlists.end()) {
            // Generate HLS segments
            fs::path segmentDir = hlsCacheDir / std::to_string(std::hash<std::string>{}(videoPath));
            std::string playlistContent;

            if (!generateHLS(fullPath, segmentDir, playlistContent)) {
                res.status = 500;
                res.set_content("Failed to generate HLS stream", "text/plain");
                return;
            }

            // Cache the playlist and segment directory
            hlsCache.playlists[videoPath] = playlistContent;
            hlsCache.segmentDirs[videoPath] = segmentDir;
        }

        // Serve cached playlist
        res.set_header("Content-Type", "application/vnd.apple.mpegurl");
        res.set_header("Cache-Control", "no-cache");
        res.set_content(hlsCache.playlists[videoPath], "application/vnd.apple.mpegurl");
    });

    // HLS segment endpoint
    server.Get(R"(/hls/(.+)/(segment\d+\.ts))", [&hlsCache](const httplib::Request& req, httplib::Response& res) {
        std::string videoPath = httplib::detail::decode_url(req.matches[1].str(), false);
        std::string segmentName = req.matches[2].str();

        // Security: prevent directory traversal
        if (videoPath.find("..") != std::string::npos || segmentName.find("..") != std::string::npos) {
            res.status = 403;
            res.set_content("Forbidden", "text/plain");
            return;
        }

        std::lock_guard<std::mutex> lock(hlsCache.mutex);

        // Check if we have segments for this video
        if (hlsCache.segmentDirs.find(videoPath) == hlsCache.segmentDirs.end()) {
            res.status = 404;
            res.set_content("Segments not found", "text/plain");
            return;
        }

        fs::path segmentPath = hlsCache.segmentDirs[videoPath] / segmentName;

        if (!fs::exists(segmentPath) || !fs::is_regular_file(segmentPath)) {
            res.status = 404;
            res.set_content("Segment not found", "text/plain");
            return;
        }

        // Serve segment file
        std::ifstream file(segmentPath, std::ios::binary);
        if (!file) {
            res.status = 500;
            res.set_content("Error reading segment", "text/plain");
            return;
        }

        file.seekg(0, std::ios::end);
        size_t fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<char> buffer(fileSize);
        file.read(buffer.data(), fileSize);

        res.set_header("Content-Type", "video/MP2T");
        res.set_header("Cache-Control", "max-age=31536000"); // Cache segments for 1 year
        res.set_content(buffer.data(), fileSize, "video/MP2T");
    });

    // Serve frontend static files
    // Try multiple paths to handle different build configurations
    std::vector<std::string> possiblePaths = {
        "../../../frontend-svelte/dist",  // Windows Visual Studio (backend/build/Release/)
        "../../frontend-svelte/dist",     // Linux/Mac (backend/build/)
        "../frontend-svelte/dist",        // Alternative
        "frontend-svelte/dist"            // Running from project root
    };

    std::string frontendPath;
    for (const auto& path : possiblePaths) {
        if (fs::exists(path) && fs::exists(fs::path(path) / "index.html")) {
            frontendPath = path;
            std::cout << "Found frontend at: " << fs::absolute(path) << std::endl;
            break;
        }
    }

    if (frontendPath.empty()) {
        std::cerr << "Warning: Frontend dist folder not found!" << std::endl;
        std::cerr << "Please run: cd frontend-svelte && npm run build" << std::endl;
        std::cerr << "Or run: build.bat (Windows) / ./build.sh (Linux/Mac)" << std::endl;
    } else {
        server.set_mount_point("/", frontendPath);
    }

    // Start server
    std::cout << "Server starting on http://" << config.host << ":" << config.port << std::endl;
    std::cout << "Access the web interface at http://localhost:" << config.port << std::endl;

    if (!server.listen(config.host, config.port)) {
        std::cerr << "Error: Failed to start server on port " << config.port << std::endl;
        return 1;
    }

    return 0;
}
