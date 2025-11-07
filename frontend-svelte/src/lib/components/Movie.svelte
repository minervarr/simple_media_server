<script lang="ts">
  import { videoPlayer, copiedVideo } from '$lib/stores/videoPlayerStore';
  import { watchedVideos } from '$lib/stores/profileStore';
  import { copyToClipboard, shareContent } from '$lib/utils/clipboard';
  import Button from './Button.svelte';
  import type { Movie as MovieType } from '$lib/types';

  export let movie: MovieType;

  $: isWatched = $watchedVideos.has(movie.path);
  $: isCopied = $copiedVideo === movie.path;

  function handlePlay() {
    videoPlayer.play(movie.path, movie.name);
    watchedVideos.markAsWatched(movie.path);
  }

  async function handleCopy() {
    const url = `/video/${movie.path}`;
    const success = await copyToClipboard(url);
    if (success) {
      copiedVideo.set(movie.path);
      setTimeout(() => copiedVideo.set(null), 2000);
    }
  }

  async function handleShare() {
    const url = `/video/${movie.path}`;
    await shareContent(url, movie.name);
  }

  function handleToggleWatched(event: MouseEvent) {
    event.stopPropagation();
    watchedVideos.toggle(movie.path);
  }
</script>

<div class="movie" class:watched={isWatched}>
  <div class="movie-name">{movie.name}</div>
  <div class="movie-actions">
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
  .movie {
    display: flex;
    flex-direction: column;
    background-color: var(--color-bg-secondary);
    border: 1px solid var(--color-border-primary);
    border-radius: var(--radius-md);
    padding: var(--spacing-xl) var(--spacing-lg);
    text-align: center;
    transition: all var(--transition-fast);
    position: relative;

    &:hover {
      background-color: var(--color-bg-tertiary);
      border-color: var(--color-border-secondary);
      transform: translateY(-2px);
    }

    &.watched {
      opacity: 0.6;

      .movie-name {
        text-decoration: line-through;
        text-decoration-color: var(--color-text-subtle);
      }
    }
  }

  .movie-name {
    font-size: var(--font-size-base);
    line-height: 1.4;
    margin-bottom: var(--spacing-md);
  }

  .movie-actions {
    display: flex;
    justify-content: center;
    gap: var(--spacing-sm);
    margin-top: auto;

    :global(.button) {
      font-size: 1.5rem;
    }
  }

  .action-button {
    background: none;
    border: none;
    font-size: 1.5rem;
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
    font-size: 1.5rem;
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
