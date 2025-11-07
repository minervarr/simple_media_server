<script lang="ts">
  import { videoPlayer, copiedVideo } from '$lib/stores/videoPlayerStore';
  import { watchedVideos } from '$lib/stores/profileStore';
  import { copyToClipboard, shareContent } from '$lib/utils/clipboard';
  import Button from './Button.svelte';
  import type { Video } from '$lib/types';

  export let episode: Video;

  $: isWatched = $watchedVideos.has(episode.path);
  $: isCopied = $copiedVideo === episode.path;

  function handlePlay() {
    videoPlayer.play(episode.path, episode.filename);
    watchedVideos.markAsWatched(episode.path);
  }

  async function handleCopy() {
    const url = `/video/${episode.path}`;
    const success = await copyToClipboard(url);
    if (success) {
      copiedVideo.set(episode.path);
      setTimeout(() => copiedVideo.set(null), 2000);
    }
  }

  async function handleShare() {
    const url = `/video/${episode.path}`;
    await shareContent(url, episode.filename);
  }

  function handleToggleWatched(event: MouseEvent) {
    event.stopPropagation();
    watchedVideos.toggle(episode.path);
  }
</script>

<div class="episode" class:watched={isWatched}>
  <div class="episode-info">
    {#if episode.episode}
      <span class="episode-number">E{episode.episode.toString().padStart(2, '0')}</span>
    {/if}
    <span class="episode-name">{episode.filename}</span>
  </div>
  <div class="episode-actions">
    <Button variant="primary" title="Play" on:click={handlePlay}>▶</Button>
    <button
      class="action-button copy-button"
      class:copied={isCopied}
      on:click={handleCopy}
      title={isCopied ? 'Link copied!' : 'Copy link'}
    >
      {isCopied ? '✓' : '🔗'}
    </button>
    <Button title="Share" on:click={handleShare}>📤</Button>
    <div
      class="watched-indicator"
      class:watched={isWatched}
      on:click={handleToggleWatched}
      on:keydown={(e) => e.key === 'Enter' && handleToggleWatched(e)}
      role="button"
      tabindex="0"
      title={isWatched ? 'Mark as unwatched' : 'Mark as watched'}
    >
      {isWatched ? '✓' : '○'}
    </div>
  </div>
</div>

<style lang="scss">
  .episode {
    display: flex;
    align-items: center;
    padding: 0.7rem 1rem 0.7rem 3rem;
    color: var(--color-text-tertiary);
    transition: background-color var(--transition-fast);
    border-left: 3px solid transparent;
    position: relative;

    &:hover {
      background-color: var(--color-bg-active);
      border-left-color: var(--color-border-tertiary);
    }

    &.watched {
      opacity: 0.6;

      .episode-name {
        text-decoration: line-through;
        text-decoration-color: var(--color-text-subtle);
      }
    }
  }

  .episode-info {
    display: flex;
    align-items: center;
    gap: var(--spacing-md);
    flex: 1;
  }

  .episode-number {
    font-size: var(--font-size-xs);
    color: var(--color-text-muted);
    font-weight: var(--font-weight-semibold);
    min-width: 2.5rem;
  }

  .episode-name {
    flex: 1;
    font-size: var(--font-size-sm);
  }

  .episode-actions {
    display: flex;
    align-items: center;
    gap: var(--spacing-sm);
  }

  .action-button {
    background: none;
    border: none;
    font-size: 1.2rem;
    cursor: pointer;
    padding: var(--spacing-xs) var(--spacing-sm);
    color: var(--color-text-disabled);
    transition: all var(--transition-fast);
    user-select: none;
    min-width: 2rem;
    text-align: center;
    border-radius: var(--radius-sm);

    &:hover {
      background-color: var(--color-bg-hover);
      color: var(--color-text-primary);
    }

    &:active {
      transform: scale(0.95);
    }
  }

  .copy-button.copied {
    color: var(--color-accent-success);
    background-color: var(--color-accent-success-bg);
  }

  .watched-indicator {
    font-size: 1.2rem;
    cursor: pointer;
    padding: var(--spacing-xs) var(--spacing-sm);
    color: var(--color-text-disabled);
    transition: color var(--transition-fast);
    user-select: none;
    min-width: 2rem;
    text-align: center;
    border-radius: var(--radius-sm);

    &:hover {
      color: var(--color-accent-success);
      background-color: var(--color-bg-hover);
    }

    &.watched {
      color: var(--color-accent-success);

      &:hover {
        color: var(--color-text-disabled);
      }
    }
  }
</style>
