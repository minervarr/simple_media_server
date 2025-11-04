#include "scanner.h"
#include <filesystem>
#include <regex>
#include <algorithm>
#include <iostream>

namespace fs = std::filesystem;

// Supported video file extensions
const std::vector<std::string> VideoScanner::videoExtensions_ = {
    ".mp4", ".mkv", ".avi", ".mov", ".wmv", ".flv", ".webm",
    ".m4v", ".mpg", ".mpeg", ".3gp", ".ogv"
};

VideoScanner::VideoScanner(const std::string& rootPath)
    : rootPath_(rootPath) {}

bool VideoScanner::isVideoFile(const std::string& filename) {
    std::string lower = filename;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    for (const auto& ext : videoExtensions_) {
        // C++17 compatible ends_with implementation
        if (lower.size() >= ext.size() &&
            lower.compare(lower.size() - ext.size(), ext.size(), ext) == 0) {
            return true;
        }
    }
    return false;
}

VideoScanner::ParsedInfo VideoScanner::parseFilename(const std::string& filename) {
    ParsedInfo info;

    // Common patterns for TV shows:
    // S01E01, S01E02, s01e01, S1E1, etc.
    // 1x01, 1x02, etc.
    // [01x01], (S01E01), etc.

    std::vector<std::regex> patterns = {
        // S01E01 or s01e01
        std::regex(R"([Ss](\d{1,2})[Ee](\d{1,3}))"),
        // 1x01
        std::regex(R"((\d{1,2})x(\d{1,3}))"),
        // Season 1 Episode 1
        std::regex(R"([Ss]eason\s*(\d{1,2}).*[Ee]pisode\s*(\d{1,3}))", std::regex::icase),
    };

    for (const auto& pattern : patterns) {
        std::smatch match;
        std::string str = filename;
        if (std::regex_search(str, match, pattern)) {
            if (match.size() >= 3) {
                info.season = std::stoi(match[1].str());
                info.episode = std::stoi(match[2].str());
                break;
            }
        }
    }

    // Clean name: remove extension and pattern info
    std::string cleanName = filename;
    size_t lastDot = cleanName.find_last_of('.');
    if (lastDot != std::string::npos) {
        cleanName = cleanName.substr(0, lastDot);
    }

    // Remove season/episode patterns
    for (const auto& pattern : patterns) {
        cleanName = std::regex_replace(cleanName, pattern, "");
    }

    // Clean up extra characters
    cleanName = std::regex_replace(cleanName, std::regex(R"([\._\-]+)"), " ");
    cleanName = std::regex_replace(cleanName, std::regex(R"(\s+)"), " ");

    // Trim
    cleanName.erase(0, cleanName.find_first_not_of(" \t\r\n"));
    cleanName.erase(cleanName.find_last_not_of(" \t\r\n") + 1);

    info.cleanName = cleanName;
    return info;
}

std::string VideoScanner::cleanSeriesName(const std::string& dirname) {
    std::string name = dirname;

    // Remove common patterns like (2020), [2020], year info, etc.
    name = std::regex_replace(name, std::regex(R"(\s*[\(\[]?\d{4}[\)\]]?\s*)"), " ");
    name = std::regex_replace(name, std::regex(R"([\._\-]+)"), " ");
    name = std::regex_replace(name, std::regex(R"(\s+)"), " ");

    // Trim
    name.erase(0, name.find_first_not_of(" \t\r\n"));
    name.erase(name.find_last_not_of(" \t\r\n") + 1);

    return name;
}

MediaLibrary VideoScanner::scan() {
    MediaLibrary library;

    if (!fs::exists(rootPath_) || !fs::is_directory(rootPath_)) {
        std::cerr << "Error: Directory does not exist: " << rootPath_ << std::endl;
        return library;
    }

    // Map to organize series: series_name -> season_number -> videos
    std::map<std::string, std::map<int, std::vector<Video>>> seriesMap;
    std::vector<Video> standaloneVideos;

    // Recursively scan directory
    for (const auto& entry : fs::recursive_directory_iterator(rootPath_)) {
        if (!entry.is_regular_file()) continue;

        std::string filename = entry.path().filename().string();
        if (!isVideoFile(filename)) continue;

        std::string relativePath = fs::relative(entry.path(), rootPath_).string();
        ParsedInfo info = parseFilename(filename);

        Video video;
        video.path = relativePath;
        video.filename = filename;
        video.season = info.season;
        video.episode = info.episode;

        // If season/episode detected, it's a series
        if (info.season.has_value() && info.episode.has_value()) {
            // Use parent directory as series name
            std::string seriesName;
            auto parentPath = entry.path().parent_path();

            // Check if parent is a season folder (e.g., "S01", "Season 1")
            std::string parentName = parentPath.filename().string();
            std::regex seasonFolderPattern(R"([Ss]eason\s*(\d+)|[Ss](\d+))", std::regex::icase);

            if (std::regex_search(parentName, seasonFolderPattern)) {
                // Parent is season folder, use grandparent as series name
                seriesName = parentPath.parent_path().filename().string();
            } else {
                // Use parent as series name
                seriesName = parentName;
            }

            seriesName = cleanSeriesName(seriesName);

            if (seriesName.empty()) {
                seriesName = "Unknown Series";
            }

            seriesMap[seriesName][*info.season].push_back(video);
        } else {
            // Standalone video (movie)
            standaloneVideos.push_back(video);
        }
    }

    // Convert series map to vector
    for (const auto& [seriesName, seasons] : seriesMap) {
        Series series;
        series.name = seriesName;
        series.displayName = seriesName; // Default display name

        for (const auto& [seasonNum, videos] : seasons) {
            Season season;
            season.number = seasonNum;
            season.episodes = videos;

            // Sort episodes by episode number
            std::sort(season.episodes.begin(), season.episodes.end(),
                [](const Video& a, const Video& b) {
                    if (a.episode.has_value() && b.episode.has_value()) {
                        return *a.episode < *b.episode;
                    }
                    return a.filename < b.filename;
                });

            series.seasons.push_back(season);
        }

        // Sort seasons
        std::sort(series.seasons.begin(), series.seasons.end(),
            [](const Season& a, const Season& b) {
                return a.number < b.number;
            });

        library.series.push_back(series);
    }

    // Sort series alphabetically
    std::sort(library.series.begin(), library.series.end(),
        [](const Series& a, const Series& b) {
            return a.name < b.name;
        });

    // Convert standalone videos to movies
    for (const auto& video : standaloneVideos) {
        Movie movie;
        movie.name = parseFilename(video.filename).cleanName;
        if (movie.name.empty()) {
            movie.name = video.filename;
        }
        movie.path = video.path;
        library.movies.push_back(movie);
    }

    // Sort movies alphabetically
    std::sort(library.movies.begin(), library.movies.end(),
        [](const Movie& a, const Movie& b) {
            return a.name < b.name;
        });

    return library;
}

json MediaLibrary::toJson() const {
    json j;

    // Series
    json seriesArray = json::array();
    for (const auto& series : series) {
        json seriesObj;
        seriesObj["name"] = series.name;
        seriesObj["displayName"] = series.displayName;

        json seasonsArray = json::array();
        for (const auto& season : series.seasons) {
            json seasonObj;
            seasonObj["number"] = season.number;

            json episodesArray = json::array();
            for (const auto& episode : season.episodes) {
                json episodeObj;
                episodeObj["path"] = episode.path;
                episodeObj["filename"] = episode.filename;
                if (episode.episode.has_value()) {
                    episodeObj["episode"] = *episode.episode;
                }
                episodesArray.push_back(episodeObj);
            }
            seasonObj["episodes"] = episodesArray;
            seasonsArray.push_back(seasonObj);
        }
        seriesObj["seasons"] = seasonsArray;
        seriesArray.push_back(seriesObj);
    }
    j["series"] = seriesArray;

    // Movies
    json moviesArray = json::array();
    for (const auto& movie : movies) {
        json movieObj;
        movieObj["name"] = movie.name;
        movieObj["path"] = movie.path;
        moviesArray.push_back(movieObj);
    }
    j["movies"] = moviesArray;

    return j;
}
