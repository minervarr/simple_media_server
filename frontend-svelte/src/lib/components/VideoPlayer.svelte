<script lang="ts">
  import { onMount, onDestroy } from 'svelte';
  import { videoPlayer } from '$lib/stores/videoPlayerStore';
  import { initializeHLSPlayer, destroyHLSPlayer } from '$lib/utils/videoPlayer';
  import type Hls from 'hls.js';

  $: player = $videoPlayer;

  let videoElement: HTMLVideoElement;
  let hlsInstance: Hls | null = null;

  onMount(() => {
    if (player && videoElement) {
      const hlsUrl = `/hls/${player.path}/playlist.m3u8`;
      hlsInstance = initializeHLSPlayer(videoElement, hlsUrl);
    }
  });

  onDestroy(() => {
    if (hlsInstance) {
      destroyHLSPlayer(hlsInstance);
    }
  });

  function handleClose() {
    videoPlayer.close();
  }

  function handleKeyDown(event: KeyboardEvent) {
    if (event.key === 'Escape') {
      handleClose();
    }
  }
</script>

{#if player}
  <!-- svelte-ignore a11y-no-noninteractive-element-interactions -->
  <div class="video-player-overlay" on:keydown={handleKeyDown} role="dialog" aria-modal="true" aria-label="Video Player">
    <div class="video-player-container">
      <div class="video-player-header">
        <h3>{player.title}</h3>
        <button class="close-button" on:click={handleClose} title="Close" aria-label="Close video player">
          ✕
        </button>
      </div>
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
  }

  .video-player-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: var(--spacing-lg) var(--spacing-xl);
    background-color: var(--color-bg-tertiary);
    border-bottom: 1px solid var(--color-border-secondary);

    h3 {
      margin: 0;
      font-size: var(--font-size-md);
      font-weight: var(--font-weight-medium);
      color: var(--color-text-primary);
      flex: 1;
      overflow: hidden;
      text-overflow: ellipsis;
      white-space: nowrap;
      padding-right: var(--spacing-lg);
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
