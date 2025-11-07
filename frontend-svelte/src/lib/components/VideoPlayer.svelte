<script lang="ts">
  import { onMount, onDestroy } from 'svelte';
  import { videoPlayer } from '$lib/stores/videoPlayerStore';
  import { initializeHLSPlayer, destroyHLSPlayer } from '$lib/utils/videoPlayer';
  import { fetchVideoInfo, getVideoUrl } from '$lib/api/videoInfo';
  import {
    loadFormatPreference,
    saveFormatPreference,
    detectDeviceCapabilities,
    getRecommendedMode
  } from '$lib/utils/formatPreferences';
  import type { VideoFileInfo, PlaybackMode } from '$lib/types';
  import type Hls from 'hls.js';

  $: player = $videoPlayer;

  let videoElement: HTMLVideoElement;
  let hlsInstance: Hls | null = null;
  let videoInfo: VideoFileInfo | null = null;
  let selectedMode: string = 'hls';
  let showFormatSelector = false;
  let loading = true;
  let loadingMessage = 'Loading video information...';

  // Device capabilities
  const deviceCapabilities = detectDeviceCapabilities();

  onMount(async () => {
    if (player && videoElement) {
      try {
        console.log('[VideoPlayer] Starting video player for:', player.path);

        // Load format preference
        const preference = loadFormatPreference();
        console.log('[VideoPlayer] Loaded preference:', preference);

        // Fetch video information
        console.log('[VideoPlayer] Fetching video info from API...');
        videoInfo = await fetchVideoInfo(player.path);

        if (videoInfo) {
          console.log('[VideoPlayer] Video info received:', {
            codec: videoInfo.video_streams[0]?.codec_name,
            resolution: `${videoInfo.video_streams[0]?.width}x${videoInfo.video_streams[0]?.height}`,
            modes: videoInfo.playback_modes.map(m => m.id)
          });

          // Auto-select format based on preference and device capabilities
          if (preference.autoSelect && videoInfo.video_streams.length > 0) {
            const videoCodec = videoInfo.video_streams[0].codec_name;
            selectedMode = getRecommendedMode(deviceCapabilities, videoCodec);
            console.log('[VideoPlayer] Auto-selected mode:', selectedMode);
          } else {
            selectedMode = preference.preferredMode;
            console.log('[VideoPlayer] Using preferred mode:', selectedMode);
          }

          // Ensure selected mode is available
          const availableModes = videoInfo.playback_modes.map((m) => m.id);
          if (!availableModes.includes(selectedMode)) {
            console.warn('[VideoPlayer] Selected mode not available, falling back to HLS');
            selectedMode = 'hls'; // Fallback to HLS
          }
        } else {
          console.error('[VideoPlayer] Failed to fetch video info, using default HLS mode');
          selectedMode = 'hls'; // Fallback to HLS if video info fails
        }

        // Initialize video player
        console.log('[VideoPlayer] Loading video with mode:', selectedMode);
        await loadVideo(selectedMode);
        console.log('[VideoPlayer] Video loaded successfully');
        loading = false;
      } catch (error) {
        console.error('[VideoPlayer] Error during initialization:', error);
        // Still try to play with HLS as fallback
        selectedMode = 'hls';
        try {
          await loadVideo(selectedMode);
        } catch (fallbackError) {
          console.error('[VideoPlayer] Fallback also failed:', fallbackError);
        }
        loading = false;
      }
    }
  });

  onDestroy(() => {
    if (hlsInstance) {
      destroyHLSPlayer(hlsInstance);
    }
  });

  async function loadVideo(mode: string) {
    if (!player || !videoElement) return;

    loadingMessage = `Loading ${mode === 'original' ? 'original quality' : mode === 'legacy' ? 'compatible format' : mode}...`;

    // Destroy existing HLS instance
    if (hlsInstance) {
      destroyHLSPlayer(hlsInstance);
      hlsInstance = null;
    }

    const videoUrl = getVideoUrl(player.path, mode);

    // For HLS modes, use HLS.js
    if (mode === 'hls') {
      hlsInstance = initializeHLSPlayer(videoElement, videoUrl);
    } else {
      // For direct video modes (original, legacy), use native video element
      videoElement.src = videoUrl;
    }
  }

  async function handleFormatChange(mode: string) {
    if (selectedMode === mode) return;

    const currentTime = videoElement.currentTime;
    const wasPlaying = !videoElement.paused;

    selectedMode = mode;
    loading = true;

    // Save preference
    const preference = loadFormatPreference();
    preference.preferredMode = mode;
    saveFormatPreference(preference);

    // Load new video source
    await loadVideo(mode);

    // Restore playback position
    videoElement.currentTime = currentTime;
    if (wasPlaying) {
      videoElement.play();
    }

    loading = false;
    showFormatSelector = false;
  }

  function toggleFormatSelector() {
    showFormatSelector = !showFormatSelector;
  }

  function handleClose() {
    videoPlayer.close();
  }

  function handleKeyDown(event: KeyboardEvent) {
    if (event.key === 'Escape') {
      if (showFormatSelector) {
        showFormatSelector = false;
      } else {
        handleClose();
      }
    }
  }

  function getCodecBadgeColor(mode: PlaybackMode): string {
    // Green for direct/no transcoding, orange for transcoding required
    if (!mode.requires_transcoding) return '#22c55e'; // Green
    return '#f59e0b'; // Orange/amber
  }

  function formatFileSize(bytes: number): string {
    if (bytes === 0) return '0 B';
    const k = 1024;
    const sizes = ['B', 'KB', 'MB', 'GB'];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return Math.round((bytes / Math.pow(k, i)) * 100) / 100 + ' ' + sizes[i];
  }

  function formatBitrate(bitrate: number): string {
    if (bitrate === 0) return 'Unknown';
    return Math.round(bitrate / 1000) + ' kbps';
  }
</script>

{#if player}
  <!-- svelte-ignore a11y-no-noninteractive-element-interactions -->
  <div class="video-player-overlay" on:keydown={handleKeyDown} role="dialog" aria-modal="true" aria-label="Video Player">
    <div class="video-player-container">
      <div class="video-player-header">
        <div class="title-section">
          <h3>{player.title}</h3>
          {#if videoInfo && videoInfo.video_streams.length > 0}
            <div class="codec-info">
              {videoInfo.video_streams[0].codec_name.toUpperCase()}
              {videoInfo.video_streams[0].width}x{videoInfo.video_streams[0].height}
              {#if videoInfo.video_streams[0].bit_depth > 8}
                {videoInfo.video_streams[0].bit_depth}-bit
              {/if}
            </div>
          {/if}
        </div>
        <div class="header-buttons">
          <button
            class="format-button"
            on:click={toggleFormatSelector}
            title="Select video format"
            aria-label="Select video format"
          >
            ⚙️ Format
          </button>
          <button class="close-button" on:click={handleClose} title="Close" aria-label="Close video player">
            ✕
          </button>
        </div>
      </div>

      <!-- Format Selector Panel -->
      {#if showFormatSelector && videoInfo}
        <div class="format-selector-panel">
          <h4>Select Playback Format</h4>
          <div class="format-options">
            {#each videoInfo.playback_modes as mode}
              <button
                class="format-option"
                class:selected={selectedMode === mode.id}
                on:click={() => handleFormatChange(mode.id)}
              >
                <div class="format-option-header">
                  <span class="format-name">{mode.name}</span>
                  <span
                    class="format-badge"
                    style="background-color: {getCodecBadgeColor(mode)}"
                  >
                    {mode.requires_transcoding ? '🔄 Transcode' : '✓ Direct'}
                  </span>
                </div>
                <div class="format-description">{mode.description}</div>
              </button>
            {/each}
          </div>

          {#if videoInfo.format}
            <div class="video-info-details">
              <h5>Video Information</h5>
              <div class="info-grid">
                <div class="info-item">
                  <span class="info-label">File Size:</span>
                  <span class="info-value">{formatFileSize(videoInfo.format.size)}</span>
                </div>
                <div class="info-item">
                  <span class="info-label">Bitrate:</span>
                  <span class="info-value">{formatBitrate(videoInfo.format.bitrate)}</span>
                </div>
                <div class="info-item">
                  <span class="info-label">Container:</span>
                  <span class="info-value">{videoInfo.format.name}</span>
                </div>
                {#if videoInfo.audio_streams.length > 0}
                  <div class="info-item">
                    <span class="info-label">Audio:</span>
                    <span class="info-value">
                      {videoInfo.audio_streams[0].codec_name.toUpperCase()}
                      {videoInfo.audio_streams[0].channels}ch
                    </span>
                  </div>
                {/if}
              </div>
            </div>
          {/if}
        </div>
      {/if}

      <!-- Loading Overlay -->
      {#if loading}
        <div class="loading-overlay">
          <div class="loading-spinner"></div>
          <p>{loadingMessage}</p>
        </div>
      {/if}

      <!-- svelte-ignore a11y-media-has-caption -->
      <video bind:this={videoElement} class="hls-video" controls autoplay />
    </div>
  </div>
{/if}

<style lang="scss">
  .video-player-overlay {
    position: fixed;
    top: 0;
    left: 0;
    right: 0;
    bottom: 0;
    background-color: rgba(0, 0, 0, 0.95);
    display: flex;
    align-items: center;
    justify-content: center;
    z-index: var(--z-modal);
    padding: var(--spacing-lg);
  }

  .video-player-container {
    background-color: var(--color-bg-secondary);
    border-radius: var(--radius-lg);
    overflow: hidden;
    width: 100%;
    max-width: 1400px;
    max-height: 90vh;
    display: flex;
    flex-direction: column;
    border: 1px solid var(--color-border-secondary);
    position: relative;
  }

  .video-player-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: var(--spacing-lg) var(--spacing-xl);
    background-color: var(--color-bg-tertiary);
    border-bottom: 1px solid var(--color-border-secondary);
  }

  .title-section {
    flex: 1;
    min-width: 0;

    h3 {
      margin: 0 0 var(--spacing-xs) 0;
      font-size: var(--font-size-md);
      font-weight: var(--font-weight-medium);
      color: var(--color-text-primary);
      overflow: hidden;
      text-overflow: ellipsis;
      white-space: nowrap;
    }

    .codec-info {
      font-size: var(--font-size-sm);
      color: var(--color-text-muted);
      font-family: monospace;
    }
  }

  .header-buttons {
    display: flex;
    gap: var(--spacing-sm);
    align-items: center;
  }

  .format-button {
    background-color: var(--color-bg-primary);
    border: 1px solid var(--color-border-secondary);
    border-radius: var(--radius-md);
    color: var(--color-text-primary);
    font-size: var(--font-size-sm);
    cursor: pointer;
    padding: var(--spacing-xs) var(--spacing-md);
    transition: all var(--transition-fast);
    white-space: nowrap;

    &:hover {
      background-color: var(--color-bg-secondary);
      border-color: var(--color-primary);
    }

    &:active {
      transform: scale(0.98);
    }
  }

  .close-button {
    background: none;
    border: none;
    color: var(--color-text-muted);
    font-size: 1.5rem;
    cursor: pointer;
    padding: var(--spacing-xs) var(--spacing-sm);
    line-height: 1;
    transition: color var(--transition-fast);
    min-width: 2rem;
    text-align: center;

    &:hover {
      color: var(--color-text-primary);
    }

    &:active {
      color: var(--color-text-secondary);
    }
  }

  .format-selector-panel {
    background-color: var(--color-bg-secondary);
    padding: var(--spacing-lg) var(--spacing-xl);
    border-bottom: 1px solid var(--color-border-secondary);
    max-height: 50vh;
    overflow-y: auto;

    h4 {
      margin: 0 0 var(--spacing-md) 0;
      font-size: var(--font-size-base);
      font-weight: var(--font-weight-medium);
      color: var(--color-text-primary);
    }

    h5 {
      margin: var(--spacing-lg) 0 var(--spacing-sm) 0;
      font-size: var(--font-size-sm);
      font-weight: var(--font-weight-medium);
      color: var(--color-text-secondary);
      text-transform: uppercase;
      letter-spacing: 0.5px;
    }
  }

  .format-options {
    display: flex;
    flex-direction: column;
    gap: var(--spacing-sm);
  }

  .format-option {
    background-color: var(--color-bg-primary);
    border: 2px solid var(--color-border-secondary);
    border-radius: var(--radius-md);
    padding: var(--spacing-md);
    cursor: pointer;
    transition: all var(--transition-fast);
    text-align: left;
    width: 100%;

    &:hover {
      border-color: var(--color-primary);
      background-color: var(--color-bg-tertiary);
    }

    &.selected {
      border-color: var(--color-primary);
      background-color: var(--color-bg-tertiary);
      box-shadow: 0 0 0 1px var(--color-primary);
    }
  }

  .format-option-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: var(--spacing-xs);
  }

  .format-name {
    font-size: var(--font-size-base);
    font-weight: var(--font-weight-medium);
    color: var(--color-text-primary);
  }

  .format-badge {
    font-size: var(--font-size-xs);
    color: white;
    padding: var(--spacing-xs) var(--spacing-sm);
    border-radius: var(--radius-sm);
    font-weight: var(--font-weight-medium);
  }

  .format-description {
    font-size: var(--font-size-sm);
    color: var(--color-text-muted);
    line-height: 1.4;
  }

  .video-info-details {
    margin-top: var(--spacing-md);
  }

  .info-grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(150px, 1fr));
    gap: var(--spacing-sm);
  }

  .info-item {
    display: flex;
    flex-direction: column;
    gap: var(--spacing-xs);
  }

  .info-label {
    font-size: var(--font-size-xs);
    color: var(--color-text-muted);
    text-transform: uppercase;
    letter-spacing: 0.5px;
  }

  .info-value {
    font-size: var(--font-size-sm);
    color: var(--color-text-primary);
    font-family: monospace;
  }

  .loading-overlay {
    position: absolute;
    top: 0;
    left: 0;
    right: 0;
    bottom: 0;
    background-color: rgba(0, 0, 0, 0.8);
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    z-index: 10;
    gap: var(--spacing-md);

    p {
      color: var(--color-text-primary);
      font-size: var(--font-size-base);
      margin: 0;
    }
  }

  .loading-spinner {
    width: 50px;
    height: 50px;
    border: 4px solid var(--color-border-secondary);
    border-top-color: var(--color-primary);
    border-radius: 50%;
    animation: spin 1s linear infinite;
  }

  @keyframes spin {
    to {
      transform: rotate(360deg);
    }
  }

  .hls-video {
    width: 100%;
    max-height: calc(90vh - 4rem);
    background-color: var(--color-bg-primary);
    outline: none;
  }

  // Mobile responsive
  @media (max-width: 768px) {
    .video-player-overlay {
      padding: 0;
    }

    .video-player-container {
      max-width: 100%;
      max-height: 100vh;
      border-radius: 0;
      border: none;
    }

    .video-player-header {
      padding: var(--spacing-lg);

      h3 {
        font-size: var(--font-size-base);
      }
    }

    .hls-video {
      max-height: calc(100vh - 3.5rem);
    }
  }
</style>
