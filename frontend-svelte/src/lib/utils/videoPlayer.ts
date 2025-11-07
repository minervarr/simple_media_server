import Hls from 'hls.js';

/**
 * Initialize HLS video player
 * Supports both HLS.js (for browsers without native HLS) and native HLS (Safari)
 */
export function initializeHLSPlayer(videoElement: HTMLVideoElement, hlsUrl: string): Hls | null {
  if (Hls.isSupported()) {
    const hls = new Hls({
      debug: false,
      enableWorker: true,
      lowLatencyMode: true,
      backBufferLength: 90,
    });

    hls.loadSource(hlsUrl);
    hls.attachMedia(videoElement);

    hls.on(Hls.Events.MANIFEST_PARSED, () => {
      videoElement.play();
    });

    hls.on(Hls.Events.ERROR, (event, data) => {
      console.error('HLS error:', data);
    });

    return hls;
  } else if (videoElement.canPlayType('application/vnd.apple.mpegurl')) {
    // Native HLS support (Safari)
    videoElement.src = hlsUrl;
    videoElement.play();
    return null;
  }

  console.error('HLS not supported in this browser');
  return null;
}

/**
 * Cleanup HLS player instance
 */
export function destroyHLSPlayer(hls: Hls | null): void {
  if (hls) {
    hls.destroy();
  }
}
