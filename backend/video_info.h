#pragma once

#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Video codec information
struct VideoCodecInfo {
    std::string codec_name;        // e.g., "h264", "hevc", "vp9", "av1"
    std::string codec_long_name;   // e.g., "H.264 / AVC / MPEG-4 AVC / MPEG-4 part 10"
    std::string profile;           // e.g., "Main", "High"
    int width = 0;
    int height = 0;
    std::string pix_fmt;           // Pixel format
    double fps = 0.0;
    int64_t bitrate = 0;
    std::string color_space;
    std::string color_transfer;
    std::string color_primaries;
    int bit_depth = 8;
};

// Audio codec information
struct AudioCodecInfo {
    std::string codec_name;        // e.g., "aac", "ac3", "dts", "flac"
    std::string codec_long_name;   // e.g., "AAC (Advanced Audio Coding)"
    int sample_rate = 0;
    int channels = 0;
    std::string channel_layout;    // e.g., "5.1", "stereo"
    int64_t bitrate = 0;
    int bit_depth = 0;
};

// Subtitle information
struct SubtitleInfo {
    std::string codec_name;
    std::string language;
    std::string title;
    bool forced = false;
};

// Container format information
struct FormatInfo {
    std::string format_name;       // e.g., "matroska,webm", "mov,mp4,m4a,3gp,3g2,mj2"
    std::string format_long_name;  // e.g., "Matroska / WebM"
    double duration = 0.0;         // Duration in seconds
    int64_t size = 0;             // File size in bytes
    int64_t bitrate = 0;          // Overall bitrate
};

// Complete video file information
struct VideoFileInfo {
    FormatInfo format;
    std::vector<VideoCodecInfo> video_streams;
    std::vector<AudioCodecInfo> audio_streams;
    std::vector<SubtitleInfo> subtitle_streams;

    // Capabilities flags
    bool is_hls_compatible = false;      // H.264/H.265 + AAC
    bool needs_video_transcode = false;  // Video codec needs transcoding
    bool needs_audio_transcode = false;  // Audio codec needs transcoding
    bool is_legacy_compatible = false;   // H.264 Baseline/Main + AAC in MP4

    // Available playback modes
    struct PlaybackMode {
        std::string id;
        std::string name;
        std::string description;
        bool requires_transcoding;
        std::string format_type;  // "original", "hls", "legacy", "custom"
    };
    std::vector<PlaybackMode> available_modes;

    // Convert to JSON for API responses
    json toJson() const;
};

// Video info analyzer using ffprobe
class VideoInfoAnalyzer {
public:
    // Analyze a video file and return detailed codec/format information
    static std::optional<VideoFileInfo> analyze(const std::string& videoPath);

private:
    // Parse ffprobe JSON output
    static std::optional<VideoFileInfo> parseFFProbeOutput(const std::string& jsonOutput);

    // Determine compatibility flags
    static void determineCompatibility(VideoFileInfo& info);

    // Generate available playback modes
    static void generatePlaybackModes(VideoFileInfo& info);

    // Helper functions
    static bool isHLSCompatibleVideoCodec(const std::string& codec);
    static bool isHLSCompatibleAudioCodec(const std::string& codec);
    static bool isLegacyCompatibleVideoCodec(const VideoCodecInfo& video);
};
