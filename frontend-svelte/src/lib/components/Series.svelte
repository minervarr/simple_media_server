<script lang="ts">
  import { expandedSeries } from '$lib/stores/libraryStore';
  import Season from './Season.svelte';
  import type { Series as SeriesType } from '$lib/types';

  export let series: SeriesType;

  $: isExpanded = $expandedSeries.has(series.name);

  function toggleSeries() {
    expandedSeries.toggle(series.name);
  }
</script>

<div class="series">
  <div class="series-header" on:click={toggleSeries} on:keydown={(e) => e.key === 'Enter' && toggleSeries()} role="button" tabindex="0">
    <span class="expand-icon">{isExpanded ? '▼' : '▶'}</span>
    <span class="series-name">{series.displayName}</span>
    <span class="series-count">{series.seasons.length} seasons</span>
  </div>

  {#if isExpanded}
    <div class="seasons">
      {#each series.seasons as season (season.number)}
        <Season {season} seriesName={series.name} />
      {/each}
    </div>
  {/if}
</div>

<style lang="scss">
  .series {
    margin-bottom: var(--spacing-lg);
    background-color: var(--color-bg-secondary);
    border-radius: var(--radius-md);
    overflow: hidden;
    border: 1px solid var(--color-border-primary);
  }

  .series-header {
    padding: var(--spacing-lg);
    cursor: pointer;
    display: flex;
    align-items: center;
    gap: var(--spacing-md);
    transition: background-color var(--transition-fast);
    user-select: none;
    outline: none;

    &:hover {
      background-color: var(--color-bg-tertiary);
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

  .series-name {
    flex: 1;
    font-size: var(--font-size-md);
    font-weight: var(--font-weight-medium);
  }

  .series-count {
    font-size: var(--font-size-sm);
    color: var(--color-text-disabled);
  }

  .seasons {
    background-color: #050505;
  }

  // Mobile responsive
  @media (max-width: 768px) {
    .series-header {
      padding: 0.8rem;
    }
  }
</style>
