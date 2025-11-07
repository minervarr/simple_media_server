import type { VideoFileInfo } from '$lib/types';

/**
 * Fetch video codec and format information from the backend
 */
export async function fetchVideoInfo(videoPath: string): Promise<VideoFileInfo | null> {
  try {
    const response = await fetch(`/api/video/info/${encodeURIComponent(videoPath)}`);

    if (!response.ok) {
      console.error('Failed to fetch video info:', response.statusText);
      return null;
    }

    const data: VideoFileInfo = await response.json();
    return data;
  } catch (error) {
    console.error('Error fetching video info:', error);
    return null;
  }
}

/**
 * Get video URL for a specific playback mode
 */
export function getVideoUrl(videoPath: string, mode: string): string {
  switch (mode) {
    case 'original':
      // Direct original file streaming
      return `/video/${videoPath}`;

    case 'hls':
      // HLS streaming (with smart transcoding)
      return `/hls/${videoPath}/playlist.m3u8`;

    case 'legacy':
      // Legacy-compatible MP4 (H.264 Baseline + AAC)
      return `/legacy/${videoPath}`;

    case 'download':
      // Direct download link
      return `/video/${videoPath}`;

    default:
      // Default to HLS
      return `/hls/${videoPath}/playlist.m3u8`;
  }
}
