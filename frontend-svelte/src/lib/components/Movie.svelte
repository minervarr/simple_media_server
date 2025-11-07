<script lang="ts">
  import { videoPlayer } from '$lib/stores/videoPlayerStore';
  import { watchedVideos } from '$lib/stores/profileStore';
  import type { Movie as MovieType } from '$lib/types';

  export let movie: MovieType;

  $: isWatched = $watchedVideos.has(movie.path);
  $: videoUrl = `/video/${movie.path}`;

  function handlePlay() {
    videoPlayer.play(movie.path, movie.name);
    watchedVideos.markAsWatched(movie.path);
  }

  function handleToggleWatched(event: MouseEvent | KeyboardEvent) {
    event.preventDefault();
    event.stopPropagation();
    watchedVideos.toggle(movie.path);
  }
</script>

<div class="movie" class:watched={isWatched}>
  <button class="movie-title" on:click={handlePlay}>
    {movie.name}
  </button>

  <div class="movie-actions">
    <a
      href={videoUrl}
      class="share-link"
      target="_blank"
      rel="noopener noreferrer"
      on:click={(e) => e.stopPropagation()}
    >
      share link
    </a>
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
    padding: var(--spacing-xl);
    transition: all var(--transition-fast);
    gap: var(--spacing-md);

    &:hover {
      background-color: var(--color-bg-tertiary);
      border-color: var(--color-border-secondary);
      transform: translateY(-2px);
      box-shadow: 0 4px 12px rgba(0, 0, 0, 0.3);
    }

    &.watched {
      opacity: 0.55;

      .movie-title {
        text-decoration: line-through;
        text-decoration-color: var(--color-text-subtle);
      }
    }
  }

  .movie-title {
    background: none;
    border: none;
    font-size: var(--font-size-md);
    line-height: 1.4;
    color: var(--color-text-primary);
    word-wrap: break-word;
    text-align: center;
    cursor: pointer;
    padding: var(--spacing-sm) 0;
    transition: color var(--transition-fast);
    font-family: inherit;
    font-weight: var(--font-weight-medium);
    flex: 1;

    &:hover {
      color: var(--color-accent-success);
    }

    &:active {
      color: var(--color-accent-success-bright);
    }
  }

  .movie-actions {
    display: flex;
    justify-content: center;
    align-items: center;
    gap: var(--spacing-lg);
    padding-top: var(--spacing-sm);
    border-top: 1px solid var(--color-border-primary);
  }

  .share-link {
    font-size: var(--font-size-sm);
    color: var(--color-text-disabled);
    text-decoration: none;
    padding: var(--spacing-xs) var(--spacing-md);
    border-radius: var(--radius-sm);
    transition: all var(--transition-fast);

    &:hover {
      color: var(--color-text-primary);
      background-color: var(--color-bg-hover);
      text-decoration: underline;
    }
  }

  .watched-indicator {
    font-size: 1.5rem;
    cursor: pointer;
    padding: var(--spacing-sm);
    color: var(--color-text-disabled);
    transition: all var(--transition-fast);
    user-select: none;
    min-width: 2.5rem;
    min-height: 2.5rem;
    display: flex;
    align-items: center;
    justify-content: center;
    border-radius: var(--radius-sm);

    &:hover {
      color: var(--color-accent-success);
      background-color: var(--color-bg-hover);
    }

    &.watched {
      color: var(--color-accent-success);

      &:hover {
        color: var(--color-text-disabled);
        background-color: var(--color-bg-hover);
      }
    }
  }

  // Mobile optimizations
  @media (max-width: 768px) {
    .movie {
      padding: var(--spacing-lg);
    }

    .movie-title {
      font-size: var(--font-size-base);
    }

    .share-link {
      font-size: var(--font-size-sm);
      padding: var(--spacing-sm) var(--spacing-lg);
    }

    .watched-indicator {
      min-width: 3rem;
      min-height: 3rem;
      font-size: 1.7rem;
    }
  }
</style>
