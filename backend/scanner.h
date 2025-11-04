#pragma once

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Represents a single video file
struct Video {
    std::string path;           // Full file path
    std::string filename;       // Original filename
    std::optional<int> season;  // Season number (if detected)
    std::optional<int> episode; // Episode number (if detected)
};

// Represents a season containing episodes
struct Season {
    int number;
    std::vector<Video> episodes;
};

// Represents a TV series with seasons
struct Series {
    std::string name;           // Series name
    std::string displayName;    // Custom display name (if set)
    std::vector<Season> seasons;
};

// Represents a standalone movie or video
struct Movie {
    std::string name;
    std::string path;
};

// Main library structure
struct MediaLibrary {
    std::vector<Series> series;
    std::vector<Movie> movies;

    json toJson() const;
};

// Video scanner class
class VideoScanner {
public:
    explicit VideoScanner(const std::string& rootPath);

    // Scan the library and organize content
    MediaLibrary scan();

    // Check if file is a video
    static bool isVideoFile(const std::string& filename);

private:
    std::string rootPath_;

    // Pattern detection
    struct ParsedInfo {
        std::optional<int> season;
        std::optional<int> episode;
        std::string cleanName;
    };

    ParsedInfo parseFilename(const std::string& filename);
    std::string cleanSeriesName(const std::string& dirname);

    // Supported video extensions
    static const std::vector<std::string> videoExtensions_;
};
