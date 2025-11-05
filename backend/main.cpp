#include <httplib.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <filesystem>
#include "scanner.h"

namespace fs = std::filesystem;
using json = nlohmann::json;

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

    // Serve frontend WASM files
    server.set_mount_point("/", "../../frontend/dist");

    // Start server
    std::cout << "Server starting on http://" << config.host << ":" << config.port << std::endl;
    std::cout << "Access the web interface at http://localhost:" << config.port << std::endl;

    if (!server.listen(config.host, config.port)) {
        std::cerr << "Error: Failed to start server on port " << config.port << std::endl;
        return 1;
    }

    return 0;
}
