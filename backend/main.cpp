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
#include "video_info.h"

namespace fs = std::filesystem;
using json = nlohmann::json;

// HLS cache for generated segments
struct HLSCache {
    std::mutex mutex;
    std::map<std::string, std::string> playlists; // video_path -> m3u8 content
    std::map<std::string, fs::path> segmentDirs;  // video_path -> segment directory
};

// Legacy compatible video cache
struct LegacyCache {
    std::mutex mutex;
    std::map<std::string, fs::path> legacyFiles;  // video_path -> legacy mp4 file
};

// Generate HLS segments for a video file with smart transcoding
bool generateHLS(const fs::path& videoPath, const fs::path& outputDir, std::string& playlistContent, bool useStreamCopy = false) {
    // Create output directory if it doesn't exist
    if (!fs::exists(outputDir)) {
        fs::create_directories(outputDir);
    }

    // Generate HLS segments using ffmpeg
    fs::path playlistPath = outputDir / "playlist.m3u8";
    fs::path segmentPattern = outputDir / "segment%d.ts";

    std::ostringstream cmd;
    cmd << "ffmpeg -i \"" << videoPath.string() << "\" ";

    if (useStreamCopy) {
        // STREAM COPY MODE: No re-encoding, preserve original quality
        // This works when the source is already H.264/H.265 + AAC/MP3
        // We need to force keyframes at segment boundaries for seeking
        std::cout << "Using stream copy (no re-encoding) for: " << videoPath << std::endl;

        cmd << "-c:v copy "              // Copy video stream without re-encoding
            << "-c:a copy "              // Copy audio stream without re-encoding
            << "-start_number 0 "
            << "-hls_time 4 "            // 4 second segments
            << "-hls_list_size 0 "       // Include all segments
            << "-hls_flags independent_segments+split_by_time " // Try to split at keyframes
            << "-hls_segment_type mpegts "
            << "-f hls ";
    } else {
        // TRANSCODE MODE: Re-encode for compatibility
        // Build ffmpeg command for HLS with proper seeking support
        // Key settings for smooth playback and seeking:
        // -c:v libx264: Re-encode video to ensure keyframes (required for seeking)
        // -g 48: Keyframe every 48 frames (2 seconds at 24fps, 1.6s at 30fps)
        // -sc_threshold 0: Disable scene change detection to keep regular keyframes
        // -c:a aac: Re-encode audio to AAC (HLS standard)
        // -hls_time 4: 4 second segments (2 keyframes per segment for smooth seeking)
        // -hls_list_size 0: Include all segments in playlist
        // -hls_flags independent_segments+split_by_time: Enable accurate seeking
        // -preset veryfast: Fast encoding without too much quality loss
        // -crf 23: Quality level (18-28 range, 23 is balanced)
        std::cout << "Transcoding to H.264/AAC for: " << videoPath << std::endl;

        cmd << "-c:v libx264 "           // H.264 video codec for compatibility
            << "-preset veryfast "       // Fast encoding
            << "-crf 23 "                // Constant quality (lower = better, 18-28 range)
            << "-g 48 "                  // Keyframe interval (2 seconds for 24fps)
            << "-sc_threshold 0 "        // Disable scene detection for regular keyframes
            << "-c:a aac "               // AAC audio for HLS compatibility
            << "-b:a 128k "              // Audio bitrate
            << "-start_number 0 "
            << "-hls_time 4 "
            << "-hls_list_size 0 "
            << "-hls_flags independent_segments+split_by_time "
            << "-hls_segment_type mpegts "
            << "-f hls ";
    }

    cmd << "\"" << playlistPath.string() << "\" 2>&1";  // Redirect stderr to stdout

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

// Generate legacy-compatible MP4 for maximum device compatibility
bool generateLegacyMP4(const fs::path& videoPath, const fs::path& outputFile) {
    // Create output directory if it doesn't exist
    fs::path outputDir = outputFile.parent_path();
    if (!fs::exists(outputDir)) {
        fs::create_directories(outputDir);
    }

    std::cout << "Generating legacy-compatible MP4 for: " << videoPath << std::endl;

    // Build ffmpeg command for legacy MP4
    // Settings for maximum compatibility:
    // - H.264 Baseline profile (most compatible, works on all devices)
    // - AAC audio (universally supported)
    // - MP4 container (widest support)
    // - Level 3.1 (supports up to 1280x720 @ 30fps or 1920x1080 @ 14fps)
    // - YUV420P pixel format (8-bit, most compatible)
    std::ostringstream cmd;
    cmd << "ffmpeg -i \"" << videoPath.string() << "\" "
        << "-c:v libx264 "                // H.264 codec
        << "-profile:v baseline "         // Baseline profile for legacy compatibility
        << "-level 3.1 "                  // Level 3.1 for wide device support
        << "-pix_fmt yuv420p "            // 8-bit color (most compatible)
        << "-preset medium "              // Balance encoding speed and quality
        << "-crf 23 "                     // Quality setting
        << "-c:a aac "                    // AAC audio
        << "-b:a 128k "                   // Audio bitrate
        << "-ac 2 "                       // Stereo audio (legacy devices may not support surround)
        << "-movflags +faststart "        // Enable fast start for web streaming
        << "-f mp4 "                      // MP4 container
        << "\"" << outputFile.string() << "\" "
        << "-y "                          // Overwrite if exists
        << "2>&1";                        // Redirect stderr to stdout

    int result = std::system(cmd.str().c_str());

    if (result != 0 || !fs::exists(outputFile)) {
        std::cerr << "Failed to generate legacy-compatible MP4" << std::endl;
        return false;
    }

    std::cout << "Legacy MP4 generation complete: " << outputFile << std::endl;
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

    // Create legacy video cache
    LegacyCache legacyCache;

    // Create legacy video cache directory
    fs::path legacyCacheDir = fs::temp_directory_path() / "media_server_legacy";
    if (!fs::exists(legacyCacheDir)) {
        fs::create_directories(legacyCacheDir);
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

    // API endpoint: Get video codec/format information
    server.Get("/api/video/info/.*", [&libPath](const httplib::Request& req, httplib::Response& res) {
        std::cout << "\n[API] ===== Video Info Request =====" << std::endl;
        std::cout << "[API] Full URL: " << req.path << std::endl;

        // Extract video path from URL
        std::string videoPath = req.path.substr(16); // Remove "/api/video/info/"
        std::cout << "[API] Extracted path: " << videoPath << std::endl;

        // Decode URL-encoded path
        videoPath = httplib::detail::decode_url(videoPath, false);
        std::cout << "[API] Decoded path: " << videoPath << std::endl;

        // Security: prevent directory traversal
        if (videoPath.find("..") != std::string::npos) {
            std::cerr << "[API] ERROR: Directory traversal attempt blocked" << std::endl;
            res.status = 403;
            res.set_content("{\"error\": \"Forbidden\"}", "application/json");
            return;
        }

        fs::path fullPath = libPath / videoPath;
        std::cout << "[API] Full file path: " << fullPath << std::endl;

        if (!fs::exists(fullPath)) {
            std::cerr << "[API] ERROR: File does not exist" << std::endl;
            res.status = 404;
            res.set_content("{\"error\": \"Video not found\"}", "application/json");
            return;
        }

        if (!fs::is_regular_file(fullPath)) {
            std::cerr << "[API] ERROR: Path is not a regular file" << std::endl;
            res.status = 404;
            res.set_content("{\"error\": \"Not a regular file\"}", "application/json");
            return;
        }

        std::cout << "[API] File exists, analyzing..." << std::endl;

        // Analyze video file
        auto videoInfo = VideoInfoAnalyzer::analyze(fullPath.string());

        if (!videoInfo) {
            std::cerr << "[API] ERROR: Failed to analyze video file" << std::endl;
            res.status = 500;
            res.set_content("{\"error\": \"Failed to analyze video file. Check if ffprobe is installed.\"}", "application/json");
            return;
        }

        std::cout << "[API] Analysis successful, sending response" << std::endl;

        // Return video info as JSON
        json response = videoInfo->toJson();
        response["file_path"] = videoPath;
        res.set_content(response.dump(), "application/json");

        std::cout << "[API] Response sent successfully" << std::endl;
        std::cout << "[API] ================================\n" << std::endl;
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

    // HLS playlist endpoint with smart transcoding
    server.Get(R"(/hls/(.+)/playlist\.m3u8)", [&libPath, &hlsCache, &hlsCacheDir](const httplib::Request& req, httplib::Response& res) {
        std::cout << "\n[HLS] ===== HLS Playlist Request =====" << std::endl;
        std::string videoPath = httplib::detail::decode_url(req.matches[1].str(), false);
        std::cout << "[HLS] Video path: " << videoPath << std::endl;

        // Security: prevent directory traversal
        if (videoPath.find("..") != std::string::npos) {
            std::cerr << "[HLS] ERROR: Directory traversal attempt blocked" << std::endl;
            res.status = 403;
            res.set_content("Forbidden", "text/plain");
            return;
        }

        fs::path fullPath = libPath / videoPath;
        std::cout << "[HLS] Full path: " << fullPath << std::endl;

        if (!fs::exists(fullPath) || !fs::is_regular_file(fullPath)) {
            std::cerr << "[HLS] ERROR: Video not found" << std::endl;
            res.status = 404;
            res.set_content("Video not found", "text/plain");
            return;
        }

        // Check cache first
        std::lock_guard<std::mutex> lock(hlsCache.mutex);

        if (hlsCache.playlists.find(videoPath) == hlsCache.playlists.end()) {
            std::cout << "[HLS] Not in cache, generating HLS stream..." << std::endl;

            // Analyze video to determine if we can use stream copy
            auto videoInfo = VideoInfoAnalyzer::analyze(fullPath.string());
            bool useStreamCopy = false;

            if (videoInfo && videoInfo->is_hls_compatible) {
                std::cout << "[HLS] ✓ Video is HLS compatible!" << std::endl;
                std::cout << "[HLS] ✓ Using STREAM COPY (zero quality loss!)" << std::endl;
                useStreamCopy = true;
            } else {
                std::cout << "[HLS] ✗ Video requires transcoding for HLS" << std::endl;
                if (videoInfo) {
                    std::cout << "[HLS]   Needs video transcode: " << (videoInfo->needs_video_transcode ? "yes" : "no") << std::endl;
                    std::cout << "[HLS]   Needs audio transcode: " << (videoInfo->needs_audio_transcode ? "yes" : "no") << std::endl;
                }
            }

            // Generate HLS segments
            fs::path segmentDir = hlsCacheDir / std::to_string(std::hash<std::string>{}(videoPath));
            std::string playlistContent;

            std::cout << "[HLS] Starting HLS generation..." << std::endl;
            if (!generateHLS(fullPath, segmentDir, playlistContent, useStreamCopy)) {
                std::cerr << "[HLS] ERROR: Failed to generate HLS stream" << std::endl;
                res.status = 500;
                res.set_content("Failed to generate HLS stream", "text/plain");
                return;
            }

            std::cout << "[HLS] HLS generation complete!" << std::endl;

            // Cache the playlist and segment directory
            hlsCache.playlists[videoPath] = playlistContent;
            hlsCache.segmentDirs[videoPath] = segmentDir;
        } else {
            std::cout << "[HLS] Serving from cache" << std::endl;
        }

        // Serve cached playlist
        res.set_header("Content-Type", "application/vnd.apple.mpegurl");
        res.set_header("Cache-Control", "no-cache");
        res.set_content(hlsCache.playlists[videoPath], "application/vnd.apple.mpegurl");

        std::cout << "[HLS] Playlist served successfully" << std::endl;
        std::cout << "[HLS] ================================\n" << std::endl;
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

    // Legacy-compatible video endpoint (H.264 Baseline + AAC MP4)
    server.Get("/legacy/.*", [&libPath, &legacyCache, &legacyCacheDir](const httplib::Request& req, httplib::Response& res) {
        // Extract video path from URL
        std::string videoPath = req.path.substr(8); // Remove "/legacy/"

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

        // Check cache first
        std::lock_guard<std::mutex> lock(legacyCache.mutex);

        if (legacyCache.legacyFiles.find(videoPath) == legacyCache.legacyFiles.end()) {
            // Check if video is already legacy-compatible
            auto videoInfo = VideoInfoAnalyzer::analyze(fullPath.string());

            if (videoInfo && videoInfo->is_legacy_compatible) {
                // Video is already compatible, serve original
                std::cout << "Video is already legacy-compatible, serving original" << std::endl;
                legacyCache.legacyFiles[videoPath] = fullPath;
            } else {
                // Generate legacy-compatible MP4
                fs::path legacyFile = legacyCacheDir / (std::to_string(std::hash<std::string>{}(videoPath)) + ".mp4");

                if (!fs::exists(legacyFile)) {
                    if (!generateLegacyMP4(fullPath, legacyFile)) {
                        res.status = 500;
                        res.set_content("Failed to generate legacy-compatible video", "text/plain");
                        return;
                    }
                }

                legacyCache.legacyFiles[videoPath] = legacyFile;
            }
        }

        fs::path legacyFilePath = legacyCache.legacyFiles[videoPath];

        // Serve the legacy file with range request support
        std::ifstream file(legacyFilePath, std::ios::binary);
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
            res.set_header("Content-Type", "video/mp4");

            // Read and send requested range
            file.seekg(start);
            std::vector<char> buffer(contentLength);
            file.read(buffer.data(), contentLength);
            res.set_content(buffer.data(), contentLength, "video/mp4");

        } else {
            // Send entire file
            res.set_header("Accept-Ranges", "bytes");
            res.set_header("Content-Length", std::to_string(fileSize));
            res.set_header("Content-Type", "video/mp4");

            std::vector<char> buffer(fileSize);
            file.read(buffer.data(), fileSize);
            res.set_content(buffer.data(), fileSize, "video/mp4");
        }
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
