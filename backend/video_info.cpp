#include "video_info.h"
#include <cstdlib>
#include <sstream>
#include <iostream>
#include <array>
#include <memory>
#include <cstring>
#include <algorithm>

// Execute command and capture output
static std::string executeCommand(const std::string& command) {
    std::array<char, 128> buffer;
    std::string result;

#ifdef _WIN32
    std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(command.c_str(), "r"), _pclose);
#else
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
#endif

    if (!pipe) {
        std::cerr << "Failed to execute command: " << command << std::endl;
        return "";
    }

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }

    return result;
}

json VideoFileInfo::toJson() const {
    json j;

    // Format info
    j["format"] = {
        {"name", format.format_name},
        {"long_name", format.format_long_name},
        {"duration", format.duration},
        {"size", format.size},
        {"bitrate", format.bitrate}
    };

    // Video streams
    j["video_streams"] = json::array();
    for (const auto& video : video_streams) {
        j["video_streams"].push_back({
            {"codec_name", video.codec_name},
            {"codec_long_name", video.codec_long_name},
            {"profile", video.profile},
            {"width", video.width},
            {"height", video.height},
            {"fps", video.fps},
            {"bitrate", video.bitrate},
            {"pix_fmt", video.pix_fmt},
            {"bit_depth", video.bit_depth}
        });
    }

    // Audio streams
    j["audio_streams"] = json::array();
    for (const auto& audio : audio_streams) {
        j["audio_streams"].push_back({
            {"codec_name", audio.codec_name},
            {"codec_long_name", audio.codec_long_name},
            {"sample_rate", audio.sample_rate},
            {"channels", audio.channels},
            {"channel_layout", audio.channel_layout},
            {"bitrate", audio.bitrate},
            {"bit_depth", audio.bit_depth}
        });
    }

    // Subtitle streams
    j["subtitle_streams"] = json::array();
    for (const auto& sub : subtitle_streams) {
        j["subtitle_streams"].push_back({
            {"codec_name", sub.codec_name},
            {"language", sub.language},
            {"title", sub.title},
            {"forced", sub.forced}
        });
    }

    // Compatibility flags
    j["compatibility"] = {
        {"is_hls_compatible", is_hls_compatible},
        {"needs_video_transcode", needs_video_transcode},
        {"needs_audio_transcode", needs_audio_transcode},
        {"is_legacy_compatible", is_legacy_compatible}
    };

    // Available playback modes
    j["playback_modes"] = json::array();
    for (const auto& mode : available_modes) {
        j["playback_modes"].push_back({
            {"id", mode.id},
            {"name", mode.name},
            {"description", mode.description},
            {"requires_transcoding", mode.requires_transcoding},
            {"format_type", mode.format_type}
        });
    }

    return j;
}

std::optional<VideoFileInfo> VideoInfoAnalyzer::analyze(const std::string& videoPath) {
    // Build ffprobe command to get JSON output
    std::ostringstream cmd;
    cmd << "ffprobe -v quiet -print_format json -show_format -show_streams "
        << "\"" << videoPath << "\" 2>&1";

    std::string output = executeCommand(cmd.str());

    if (output.empty()) {
        std::cerr << "Failed to get ffprobe output for: " << videoPath << std::endl;
        return std::nullopt;
    }

    return parseFFProbeOutput(output);
}

std::optional<VideoFileInfo> VideoInfoAnalyzer::parseFFProbeOutput(const std::string& jsonOutput) {
    try {
        json data = json::parse(jsonOutput);
        VideoFileInfo info;

        // Parse format information
        if (data.contains("format")) {
            auto& fmt = data["format"];
            info.format.format_name = fmt.value("format_name", "");
            info.format.format_long_name = fmt.value("format_long_name", "");
            info.format.duration = std::stod(fmt.value("duration", "0"));
            info.format.size = std::stoll(fmt.value("size", "0"));
            info.format.bitrate = std::stoll(fmt.value("bit_rate", "0"));
        }

        // Parse streams
        if (data.contains("streams") && data["streams"].is_array()) {
            for (const auto& stream : data["streams"]) {
                std::string codec_type = stream.value("codec_type", "");

                if (codec_type == "video") {
                    VideoCodecInfo video;
                    video.codec_name = stream.value("codec_name", "");
                    video.codec_long_name = stream.value("codec_long_name", "");
                    video.profile = stream.value("profile", "");
                    video.width = stream.value("width", 0);
                    video.height = stream.value("height", 0);
                    video.pix_fmt = stream.value("pix_fmt", "");
                    video.bitrate = std::stoll(stream.value("bit_rate", "0"));

                    // Parse FPS
                    std::string r_frame_rate = stream.value("r_frame_rate", "0/1");
                    size_t slash_pos = r_frame_rate.find('/');
                    if (slash_pos != std::string::npos) {
                        double num = std::stod(r_frame_rate.substr(0, slash_pos));
                        double den = std::stod(r_frame_rate.substr(slash_pos + 1));
                        if (den > 0) video.fps = num / den;
                    }

                    // Color information
                    video.color_space = stream.value("color_space", "");
                    video.color_transfer = stream.value("color_transfer", "");
                    video.color_primaries = stream.value("color_primaries", "");

                    // Bit depth
                    if (stream.contains("bits_per_raw_sample")) {
                        video.bit_depth = stream.value("bits_per_raw_sample", 8);
                    } else {
                        // Try to infer from pix_fmt
                        if (video.pix_fmt.find("10") != std::string::npos) {
                            video.bit_depth = 10;
                        } else if (video.pix_fmt.find("12") != std::string::npos) {
                            video.bit_depth = 12;
                        }
                    }

                    info.video_streams.push_back(video);

                } else if (codec_type == "audio") {
                    AudioCodecInfo audio;
                    audio.codec_name = stream.value("codec_name", "");
                    audio.codec_long_name = stream.value("codec_long_name", "");
                    audio.sample_rate = stream.value("sample_rate", 0);
                    audio.channels = stream.value("channels", 0);
                    audio.channel_layout = stream.value("channel_layout", "");
                    audio.bitrate = std::stoll(stream.value("bit_rate", "0"));
                    audio.bit_depth = stream.value("bits_per_sample", 0);

                    info.audio_streams.push_back(audio);

                } else if (codec_type == "subtitle") {
                    SubtitleInfo sub;
                    sub.codec_name = stream.value("codec_name", "");
                    if (stream.contains("tags")) {
                        auto& tags = stream["tags"];
                        sub.language = tags.value("language", "");
                        sub.title = tags.value("title", "");
                    }
                    sub.forced = stream.value("disposition", json::object()).value("forced", 0) == 1;

                    info.subtitle_streams.push_back(sub);
                }
            }
        }

        // Determine compatibility and available modes
        determineCompatibility(info);
        generatePlaybackModes(info);

        return info;

    } catch (const std::exception& e) {
        std::cerr << "Error parsing ffprobe output: " << e.what() << std::endl;
        return std::nullopt;
    }
}

bool VideoInfoAnalyzer::isHLSCompatibleVideoCodec(const std::string& codec) {
    return codec == "h264" || codec == "hevc" || codec == "h265";
}

bool VideoInfoAnalyzer::isHLSCompatibleAudioCodec(const std::string& codec) {
    return codec == "aac" || codec == "mp3";
}

bool VideoInfoAnalyzer::isLegacyCompatibleVideoCodec(const VideoCodecInfo& video) {
    if (video.codec_name != "h264") return false;

    // Legacy devices typically support H.264 Baseline/Main profiles, 8-bit
    std::string profile_lower = video.profile;
    std::transform(profile_lower.begin(), profile_lower.end(), profile_lower.begin(), ::tolower);

    return (profile_lower.find("baseline") != std::string::npos ||
            profile_lower.find("main") != std::string::npos) &&
           video.bit_depth <= 8;
}

void VideoInfoAnalyzer::determineCompatibility(VideoFileInfo& info) {
    // Check HLS compatibility (H.264/H.265 video + AAC/MP3 audio)
    bool has_hls_video = false;
    bool has_non_hls_video = false;

    for (const auto& video : info.video_streams) {
        if (isHLSCompatibleVideoCodec(video.codec_name)) {
            has_hls_video = true;
        } else {
            has_non_hls_video = true;
        }
    }

    bool has_hls_audio = false;
    bool has_non_hls_audio = false;

    for (const auto& audio : info.audio_streams) {
        if (isHLSCompatibleAudioCodec(audio.codec_name)) {
            has_hls_audio = true;
        } else {
            has_non_hls_audio = true;
        }
    }

    info.is_hls_compatible = has_hls_video && has_hls_audio;
    info.needs_video_transcode = has_non_hls_video || !has_hls_video;
    info.needs_audio_transcode = has_non_hls_audio || !has_hls_audio;

    // Check legacy compatibility (H.264 Baseline/Main + AAC in MP4)
    bool has_legacy_video = false;
    for (const auto& video : info.video_streams) {
        if (isLegacyCompatibleVideoCodec(video)) {
            has_legacy_video = true;
            break;
        }
    }

    bool is_mp4 = info.format.format_name.find("mp4") != std::string::npos;
    info.is_legacy_compatible = has_legacy_video && has_hls_audio && is_mp4;
}

void VideoInfoAnalyzer::generatePlaybackModes(VideoFileInfo& info) {
    info.available_modes.clear();

    // Mode 1: Original Quality (always available, no transcoding)
    VideoFileInfo::PlaybackMode originalMode;
    originalMode.id = "original";
    originalMode.name = "Original Quality";
    originalMode.requires_transcoding = false;
    originalMode.format_type = "original";

    if (!info.video_streams.empty()) {
        auto& video = info.video_streams[0];
        std::ostringstream desc;
        desc << video.codec_name << " " << video.width << "x" << video.height;
        if (video.bit_depth > 8) {
            desc << " " << video.bit_depth << "-bit";
        }
        if (!info.audio_streams.empty()) {
            desc << " + " << info.audio_streams[0].codec_name;
        }
        desc << " (No transcoding, best quality)";
        originalMode.description = desc.str();
    } else {
        originalMode.description = "Original file without any transcoding";
    }

    info.available_modes.push_back(originalMode);

    // Mode 2: HLS Streaming (recommended for web playback)
    VideoFileInfo::PlaybackMode hlsMode;
    hlsMode.id = "hls";
    hlsMode.name = "HLS Streaming";
    hlsMode.format_type = "hls";

    if (info.is_hls_compatible) {
        hlsMode.requires_transcoding = false;
        hlsMode.description = "Stream copy (no re-encoding) - Best quality with seeking support";
    } else {
        hlsMode.requires_transcoding = true;
        std::ostringstream desc;
        desc << "Transcode to H.264/AAC";
        if (info.needs_video_transcode) desc << " (video)";
        if (info.needs_audio_transcode) desc << " (audio)";
        desc << " - Recommended for web browsers";
        hlsMode.description = desc.str();
    }

    info.available_modes.push_back(hlsMode);

    // Mode 3: Legacy Compatible (for old devices)
    VideoFileInfo::PlaybackMode legacyMode;
    legacyMode.id = "legacy";
    legacyMode.name = "Legacy Compatible";
    legacyMode.format_type = "legacy";

    if (info.is_legacy_compatible) {
        legacyMode.requires_transcoding = false;
        legacyMode.description = "H.264 Baseline/Main + AAC in MP4 (Direct play on all devices)";
    } else {
        legacyMode.requires_transcoding = true;
        legacyMode.description = "Transcode to H.264 Baseline + AAC MP4 - Compatible with all devices (old TVs, phones)";
    }

    info.available_modes.push_back(legacyMode);

    // Mode 4: Direct Download (for native players)
    VideoFileInfo::PlaybackMode downloadMode;
    downloadMode.id = "download";
    downloadMode.name = "Direct Download/Link";
    downloadMode.requires_transcoding = false;
    downloadMode.format_type = "original";
    downloadMode.description = "Direct link to original file - For native video players";

    info.available_modes.push_back(downloadMode);
}
