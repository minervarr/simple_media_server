<script lang="ts">
  import { videoPlayer } from '$lib/stores/videoPlayerStore';
  import { watchedVideos } from '$lib/stores/profileStore';
  import type { Video } from '$lib/types';

  export let episode: Video;

  $: isWatched = $watchedVideos.has(episode.path);
  $: videoUrl = `/video/${episode.path}`;

  function handlePlay() {
    videoPlayer.play(episode.path, episode.filename);
    watchedVideos.markAsWatched(episode.path);
  }

  function handleToggleWatched(event: MouseEvent | KeyboardEvent) {
    event.preventDefault();
    event.stopPropagation();
    watchedVideos.toggle(episode.path);
  }
</script>

<div class="episode" class:watched={isWatched}>
  <div class="episode-main">
    {#if episode.episode}
      <span class="episode-number">E{episode.episode.toString().padStart(2, '0')}</span>
    {/if}
    <button class="episode-title" on:click={handlePlay}>
      {episode.filename}
    </button>
  </div>

  <div class="episode-actions">
    <a
      href={videoUrl}
      class="share-link"
      target="_blank"
      rel="noopener noreferrer"
      on:click={(e) => e.stopPropagation()}
    >
      link
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
    justify-content: space-between;
    padding: 0.85rem 1.25rem 0.85rem 2rem;
    gap: var(--spacing-lg);
    transition: background-color var(--transition-fast);
    border-left: 3px solid transparent;

    &:hover {
      background-color: var(--color-bg-active);
      border-left-color: var(--color-accent-success);
    }

    &.watched {
      opacity: 0.55;

      .episode-title {
        text-decoration: line-through;
        text-decoration-color: var(--color-text-subtle);
      }
    }
  }

  .episode-main {
    display: flex;
    align-items: center;
    gap: var(--spacing-md);
    flex: 1;
    min-width: 0;
  }

  .episode-number {
    font-size: var(--font-size-xs);
    color: var(--color-text-muted);
    font-weight: var(--font-weight-semibold);
    min-width: 2.5rem;
    flex-shrink: 0;
    letter-spacing: 0.5px;
  }

  .episode-title {
    flex: 1;
    font-size: var(--font-size-base);
    color: var(--color-text-primary);
    text-align: left;
    background: none;
    border: none;
    padding: var(--spacing-xs) 0;
    cursor: pointer;
    transition: color var(--transition-fast);
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
    font-family: inherit;

    &:hover {
      color: var(--color-accent-success);
    }

    &:active {
      color: var(--color-accent-success-bright);
    }
  }

  .episode-actions {
    display: flex;
    align-items: center;
    gap: var(--spacing-md);
    flex-shrink: 0;
  }

  .share-link {
    font-size: var(--font-size-sm);
    color: var(--color-text-disabled);
    text-decoration: none;
    padding: var(--spacing-xs) var(--spacing-sm);
    border-radius: var(--radius-sm);
    transition: all var(--transition-fast);
    white-space: nowrap;

    &:hover {
      color: var(--color-text-primary);
      background-color: var(--color-bg-hover);
      text-decoration: underline;
    }
  }

  .watched-indicator {
    font-size: 1.3rem;
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
    .episode {
      padding: 1.1rem 1rem 1.1rem 1.5rem;
    }

    .episode-title {
      font-size: var(--font-size-base);
    }

    .share-link {
      font-size: var(--font-size-sm);
      padding: var(--spacing-sm) var(--spacing-md);
    }

    .watched-indicator {
      min-width: 3rem;
      min-height: 3rem;
      font-size: 1.5rem;
    }
  }

  @media (max-width: 480px) {
    .episode {
      padding: 1rem 0.75rem 1rem 1rem;
    }

    .episode-number {
      min-width: 2rem;
      font-size: 0.7rem;
    }
  }
</style>
