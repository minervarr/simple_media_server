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
      // Optimized for seeking and smooth playback
      lowLatencyMode: false,           // Disable low latency for better seeking
      backBufferLength: 90,             // Keep 90 seconds of back buffer
      maxBufferLength: 30,              // Load 30 seconds ahead
      maxMaxBufferLength: 60,           // Maximum buffer size
      maxBufferSize: 60 * 1000 * 1000,  // 60 MB max buffer
      maxBufferHole: 0.5,               // Gap tolerance for seeking
      highBufferWatchdogPeriod: 2,      // Check buffer health every 2s
      nudgeMaxRetry: 3,                 // Retry seeking if stuck
      enableWorker: true,               // Use web worker for better performance
      startPosition: -1,                // Start from beginning
    });

    hls.loadSource(hlsUrl);
    hls.attachMedia(videoElement);

    hls.on(Hls.Events.MANIFEST_PARSED, () => {
      // Don't auto-play, let user control
      // videoElement.play();
    });

    hls.on(Hls.Events.ERROR, (event, data) => {
      console.error('HLS error:', data);

      // Handle fatal errors
      if (data.fatal) {
        switch (data.type) {
          case Hls.ErrorTypes.NETWORK_ERROR:
            console.log('Network error, trying to recover...');
            hls.startLoad();
            break;
          case Hls.ErrorTypes.MEDIA_ERROR:
            console.log('Media error, trying to recover...');
            hls.recoverMediaError();
            break;
          default:
            console.error('Fatal error, cannot recover');
            hls.destroy();
            break;
        }
      }
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
