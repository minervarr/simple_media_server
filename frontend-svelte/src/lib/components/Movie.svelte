<script lang="ts">
  import { watchedVideos } from '$lib/stores/profileStore';
  import type { Movie as MovieType } from '$lib/types';

  export let movie: MovieType;

  $: isWatched = $watchedVideos.has(movie.path);
  $: videoUrl = `/video/${movie.path}`;

  function handleToggleWatched(event: MouseEvent) {
    event.preventDefault();
    event.stopPropagation();
    watchedVideos.toggle(movie.path);
  }

  function handleLinkClick() {
    // Mark as watched when link is clicked
    watchedVideos.markAsWatched(movie.path);
  }
</script>

<div class="movie" class:watched={isWatched}>
  <a
    href={videoUrl}
    class="movie-link"
    on:click={handleLinkClick}
    target="_blank"
    rel="noopener noreferrer"
  >
    <div class="movie-name">{movie.name}</div>
  </a>
  <div class="movie-actions">
    <a
      href={videoUrl}
      class="play-link"
      on:click={handleLinkClick}
      target="_blank"
      rel="noopener noreferrer"
    >
      Play
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

  .movie-link {
    text-decoration: none;
    color: inherit;
    display: block;
    margin-bottom: var(--spacing-md);
    transition: color var(--transition-fast);

    &:hover {
      color: var(--color-accent-success);
    }
  }

  .movie-name {
    font-size: var(--font-size-md);
    line-height: 1.4;
    color: var(--color-text-primary);
    word-wrap: break-word;
    transition: color var(--transition-fast);
  }

  .movie-actions {
    display: flex;
    justify-content: center;
    align-items: center;
    gap: var(--spacing-lg);
    margin-top: auto;
  }

  .play-link {
    font-size: var(--font-size-base);
    color: var(--color-accent-success);
    text-decoration: none;
    padding: var(--spacing-sm) var(--spacing-xl);
    border-radius: var(--radius-sm);
    transition: all var(--transition-fast);
    font-weight: var(--font-weight-medium);

    &:hover {
      background-color: var(--color-accent-success-bg);
      color: var(--color-accent-success-bright);
    }

    &:active {
      transform: scale(0.95);
    }
  }

  .watched-indicator {
    font-size: 1.5rem;
    cursor: pointer;
    padding: var(--spacing-sm);
    color: var(--color-text-disabled);
    transition: color var(--transition-fast);
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
      }
    }
  }

  // Mobile optimizations
  @media (max-width: 768px) {
    .movie {
      padding: var(--spacing-lg);
    }

    .movie-name {
      font-size: var(--font-size-base);
    }

    .play-link {
      padding: var(--spacing-sm) var(--spacing-lg);
      font-size: var(--font-size-sm);
    }

    .watched-indicator {
      min-width: 3rem;
      min-height: 3rem;
      font-size: 1.7rem;
    }
  }
</style>
