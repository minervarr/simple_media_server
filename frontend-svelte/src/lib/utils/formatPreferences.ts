import type { FormatPreference } from '$lib/types';

const STORAGE_KEY = 'video_format_preference';

// Default preference
const DEFAULT_PREFERENCE: FormatPreference = {
  preferredMode: 'hls',  // Default to HLS for web compatibility
  autoSelect: true       // Auto-select based on device capabilities
};

/**
 * Load format preference from localStorage
 */
export function loadFormatPreference(): FormatPreference {
  try {
    const stored = localStorage.getItem(STORAGE_KEY);
    if (stored) {
      return JSON.parse(stored);
    }
  } catch (error) {
    console.error('Failed to load format preference:', error);
  }
  return DEFAULT_PREFERENCE;
}

/**
 * Save format preference to localStorage
 */
export function saveFormatPreference(preference: FormatPreference): void {
  try {
    localStorage.setItem(STORAGE_KEY, JSON.stringify(preference));
  } catch (error) {
    console.error('Failed to save format preference:', error);
  }
}

/**
 * Detect device capabilities for codec support
 */
export function detectDeviceCapabilities() {
  const video = document.createElement('video');

  const capabilities = {
    supportsHEVC: false,
    supportsVP9: false,
    supportsAV1: false,
    supportsH264: false,
    supportsHLS: false
  };

  // Check H.264 support (baseline for most devices)
  capabilities.supportsH264 = video.canPlayType('video/mp4; codecs="avc1.42E01E"') !== '';

  // Check HEVC/H.265 support (modern devices, Safari on Apple Silicon)
  capabilities.supportsHEVC =
    video.canPlayType('video/mp4; codecs="hev1.1.6.L93.B0"') !== '' ||
    video.canPlayType('video/mp4; codecs="hvc1.1.6.L93.B0"') !== '';

  // Check VP9 support (Chrome, Firefox, modern Edge)
  capabilities.supportsVP9 = video.canPlayType('video/webm; codecs="vp9"') !== '';

  // Check AV1 support (very modern browsers)
  capabilities.supportsAV1 = video.canPlayType('video/mp4; codecs="av01.0.05M.08"') !== '';

  // Check HLS support (native on Safari, via hls.js on others)
  capabilities.supportsHLS =
    video.canPlayType('application/vnd.apple.mpegurl') !== '' ||
    video.canPlayType('application/x-mpegURL') !== '';

  return capabilities;
}

/**
 * Get recommended playback mode based on device capabilities and video info
 */
export function getRecommendedMode(
  capabilities: ReturnType<typeof detectDeviceCapabilities>,
  videoCodec?: string
): string {
  // If device has native HLS support (Safari), use HLS
  if (capabilities.supportsHLS) {
    return 'hls';
  }

  // If video is already in a compatible format, use original
  if (videoCodec === 'h264' || videoCodec === 'hevc' && capabilities.supportsHEVC) {
    return 'original';
  }

  // Default to HLS with hls.js transcoding
  return 'hls';
}
