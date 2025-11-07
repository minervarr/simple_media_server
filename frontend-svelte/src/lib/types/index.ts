// Core data types matching backend JSON structures

export interface Profile {
  id: string;
  name: string;
  icon: string;
}

export interface Video {
  path: string;
  filename: string;
  episode?: number;
}

export interface Season {
  number: number;
  episodes: Video[];
}

export interface Series {
  name: string;
  displayName: string;
  seasons: Season[];
}

export interface Movie {
  name: string;
  path: string;
}

export interface Library {
  series: Series[];
  movies: Movie[];
}

export interface VideoPlayerInfo {
  path: string;
  title: string;
}

// Video codec and format information types
export interface VideoCodecInfo {
  codec_name: string;
  codec_long_name: string;
  profile: string;
  width: number;
  height: number;
  fps: number;
  bitrate: number;
  pix_fmt: string;
  bit_depth: number;
}

export interface AudioCodecInfo {
  codec_name: string;
  codec_long_name: string;
  sample_rate: number;
  channels: number;
  channel_layout: string;
  bitrate: number;
  bit_depth: number;
}

export interface SubtitleInfo {
  codec_name: string;
  language: string;
  title: string;
  forced: boolean;
}

export interface FormatInfo {
  name: string;
  long_name: string;
  duration: number;
  size: number;
  bitrate: number;
}

export interface PlaybackMode {
  id: string;
  name: string;
  description: string;
  requires_transcoding: boolean;
  format_type: string;
}

export interface VideoFileInfo {
  file_path: string;
  format: FormatInfo;
  video_streams: VideoCodecInfo[];
  audio_streams: AudioCodecInfo[];
  subtitle_streams: SubtitleInfo[];
  compatibility: {
    is_hls_compatible: boolean;
    needs_video_transcode: boolean;
    needs_audio_transcode: boolean;
    is_legacy_compatible: boolean;
  };
  playback_modes: PlaybackMode[];
}

// Format preference types
export interface FormatPreference {
  preferredMode: string; // "original", "hls", "legacy", "download"
  autoSelect: boolean;    // Auto-select based on device capabilities
}
