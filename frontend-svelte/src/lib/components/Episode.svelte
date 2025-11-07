<script lang="ts">
  import { watchedVideos } from '$lib/stores/profileStore';
  import type { Video } from '$lib/types';

  export let episode: Video;

  $: isWatched = $watchedVideos.has(episode.path);
  $: videoUrl = `/video/${episode.path}`;

  function handleToggleWatched(event: MouseEvent) {
    event.preventDefault();
    event.stopPropagation();
    watchedVideos.toggle(episode.path);
  }

  function handleLinkClick(event: MouseEvent) {
    // Mark as watched when link is clicked
    watchedVideos.markAsWatched(episode.path);
  }
</script>

<div class="episode" class:watched={isWatched}>
  <div class="episode-info">
    {#if episode.episode}
      <span class="episode-number">E{episode.episode.toString().padStart(2, '0')}</span>
    {/if}
    <a
      href={videoUrl}
      class="episode-link"
      on:click={handleLinkClick}
      target="_blank"
      rel="noopener noreferrer"
    >
      {episode.filename}
    </a>
  </div>
  <div class="episode-actions">
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
  .episode {
    display: flex;
    align-items: center;
    padding: 0.9rem 1rem 0.9rem 2.5rem;
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

      .episode-link {
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
    min-width: 0; // Allow text truncation
  }

  .episode-number {
    font-size: var(--font-size-xs);
    color: var(--color-text-muted);
    font-weight: var(--font-weight-semibold);
    min-width: 2.5rem;
    flex-shrink: 0;
  }

  .episode-link {
    flex: 1;
    font-size: var(--font-size-base);
    color: var(--color-text-primary);
    text-decoration: none;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
    padding: var(--spacing-xs) 0;
    transition: color var(--transition-fast);

    &:hover {
      color: var(--color-accent-success);
      text-decoration: underline;
    }

    &:active {
      color: var(--color-accent-success-bright);
    }
  }

  .episode-actions {
    display: flex;
    align-items: center;
    gap: var(--spacing-lg);
    flex-shrink: 0;
  }

  .play-link {
    font-size: var(--font-size-base);
    color: var(--color-accent-success);
    text-decoration: none;
    padding: var(--spacing-sm) var(--spacing-lg);
    border-radius: var(--radius-sm);
    transition: all var(--transition-fast);
    font-weight: var(--font-weight-medium);
    white-space: nowrap;

    &:hover {
      background-color: var(--color-accent-success-bg);
      color: var(--color-accent-success-bright);
    }

    &:active {
      transform: scale(0.95);
    }
  }

  .watched-indicator {
    font-size: 1.3rem;
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
    .episode {
      padding: 1rem 0.75rem 1rem 1.5rem;
      flex-wrap: wrap;
      gap: var(--spacing-sm);
    }

    .episode-info {
      width: 100%;
    }

    .episode-link {
      font-size: var(--font-size-base);
    }

    .play-link {
      padding: var(--spacing-sm) var(--spacing-md);
      font-size: var(--font-size-sm);
    }

    .watched-indicator {
      min-width: 3rem;
      min-height: 3rem;
      font-size: 1.5rem;
    }
  }
</style>
