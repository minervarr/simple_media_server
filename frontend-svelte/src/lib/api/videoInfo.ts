import type { VideoFileInfo } from '$lib/types';

/**
 * Fetch video codec and format information from the backend
 */
export async function fetchVideoInfo(videoPath: string): Promise<VideoFileInfo | null> {
  const url = `/api/video/info/${encodeURIComponent(videoPath)}`;
  console.log('[API] Fetching video info:', url);

  try {
    const response = await fetch(url);

    console.log('[API] Response status:', response.status, response.statusText);

    if (!response.ok) {
      const errorText = await response.text();
      console.error('[API] Failed to fetch video info:', {
        status: response.status,
        statusText: response.statusText,
        body: errorText
      });
      return null;
    }

    const data: VideoFileInfo = await response.json();
    console.log('[API] Video info fetched successfully:', {
      videoStreams: data.video_streams.length,
      audioStreams: data.audio_streams.length,
      playbackModes: data.playback_modes.length
    });
    return data;
  } catch (error) {
    console.error('[API] Error fetching video info:', error);
    return null;
  }
}

/**
 * Get video URL for a specific playback mode
 * Note: Paths are NOT encoded here because the browser's fetch API and video element
 * handle encoding automatically for URLs. The backend decodes them properly.
 */
export function getVideoUrl(videoPath: string, mode: string): string {
  // Encode each path segment separately to handle special characters
  const encodedPath = videoPath.split('/').map(segment => encodeURIComponent(segment)).join('/');

  switch (mode) {
    case 'original':
      // Direct original file streaming
      return `/video/${encodedPath}`;

    case 'hls':
      // HLS streaming (with smart transcoding)
      return `/hls/${encodedPath}/playlist.m3u8`;

    case 'legacy':
      // Legacy-compatible MP4 (H.264 Baseline + AAC)
      return `/legacy/${encodedPath}`;

    case 'download':
      // Direct download link
      return `/video/${encodedPath}`;

    default:
      // Default to HLS
      return `/hls/${encodedPath}/playlist.m3u8`;
  }
}
