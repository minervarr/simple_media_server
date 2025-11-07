<script lang="ts">
  import { expandedSeasons } from '$lib/stores/libraryStore';
  import Episode from './Episode.svelte';
  import type { Season as SeasonType } from '$lib/types';

  export let season: SeasonType;
  export let seriesName: string;

  $: seasonKey = `${seriesName}_${season.number}`;
  $: isExpanded = $expandedSeasons.has(seasonKey);

  function toggleSeason() {
    expandedSeasons.toggle(seriesName, season.number);
  }
</script>

<div class="season">
  <div class="season-header" on:click={toggleSeason} on:keydown={(e) => e.key === 'Enter' && toggleSeason()} role="button" tabindex="0">
    <span class="expand-icon">{isExpanded ? '▼' : '▶'}</span>
    <span class="season-name">Season {season.number}</span>
    <span class="episode-count">{season.episodes.length} episodes</span>
  </div>

  {#if isExpanded}
    <div class="episodes">
      {#each season.episodes as episode (episode.path)}
        <Episode {episode} />
      {/each}
    </div>
  {/if}
</div>

<style lang="scss">
  .season {
    border-top: 1px solid var(--color-border-primary);
  }

  .season-header {
    padding: 0.9rem 1rem 0.9rem 2rem;
    cursor: pointer;
    display: flex;
    align-items: center;
    gap: var(--spacing-md);
    transition: background-color var(--transition-fast);
    user-select: none;
    outline: none;

    &:hover {
      background-color: var(--color-bg-active);
    }

    &:focus-visible {
      outline: 2px solid var(--color-border-tertiary);
      outline-offset: -2px;
    }
  }

  .expand-icon {
    font-size: 0.8rem;
    color: var(--color-text-muted);
    min-width: 1rem;
  }

  .season-name {
    flex: 1;
    font-size: var(--font-size-base);
  }

  .episode-count {
    font-size: var(--font-size-xs);
    color: var(--color-text-subtle);
  }

  .episodes {
    background-color: var(--color-bg-primary);
    padding: var(--spacing-sm) 0;
  }
</style>
