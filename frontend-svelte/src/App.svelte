<script lang="ts">
  import { onMount } from 'svelte';
  import { profiles, showProfileSelector } from '$lib/stores/profileStore';
  import { library, loading, error, filteredLibrary } from '$lib/stores/libraryStore';
  import ProfileSelector from '$lib/components/ProfileSelector.svelte';
  import Header from '$lib/components/Header.svelte';
  import Series from '$lib/components/Series.svelte';
  import Movie from '$lib/components/Movie.svelte';
  import VideoPlayer from '$lib/components/VideoPlayer.svelte';
  import type { Profile } from '$lib/types';

  $: showSelector = $showProfileSelector;
  $: isLoading = $loading;
  $: errorMessage = $error;
  $: libraryData = $filteredLibrary;

  onMount(() => {
    // Load profiles and library on app start
    profiles.load();
    library.load();
  });

  function handleProfileSelect(profile: Profile) {
    profiles.selectProfile(profile);
  }

  function handleShowProfileSelector() {
    profiles.showSelector();
  }
</script>

<div class="app">
  {#if showSelector}
    <ProfileSelector onSelect={handleProfileSelect} />
  {:else}
    <Header onShowProfileSelector={handleShowProfileSelector} />

    <main>
      {#if isLoading}
        <div class="loading">Loading library...</div>
      {:else if errorMessage}
        <div class="error">Error: {errorMessage}</div>
      {:else if libraryData}
        {#if libraryData.series.length > 0}
          <section class="series-section">
            <h2>TV Series</h2>
            {#each libraryData.series as series (series.name)}
              <Series {series} />
            {/each}
          </section>
        {/if}

        {#if libraryData.movies.length > 0}
          <section class="movies-section">
            <h2>Movies</h2>
            <div class="movie-list">
              {#each libraryData.movies as movie (movie.path)}
                <Movie {movie} />
              {/each}
            </div>
          </section>
        {/if}

        {#if libraryData.series.length === 0 && libraryData.movies.length === 0}
          <div class="no-results">No results found</div>
        {/if}
      {:else}
        <div class="error">No library data available</div>
      {/if}
    </main>

    <VideoPlayer />
  {/if}
</div>

<style lang="scss">
  .app {
    min-height: 100vh;
    display: flex;
    flex-direction: column;
  }

  main {
    flex: 1;
    padding: var(--spacing-xl);
    max-width: 1200px;
    width: 100%;
    margin: 0 auto;
  }

  .loading,
  .error,
  .no-results {
    text-align: center;
    padding: var(--spacing-3xl) var(--spacing-lg);
    color: var(--color-text-muted);
    font-size: var(--font-size-md);
  }

  .error {
    color: var(--color-accent-error);
  }

  .series-section,
  .movies-section {
    margin-bottom: var(--spacing-2xl);
  }

  h2 {
    font-size: var(--font-size-xl);
    margin-bottom: var(--spacing-lg);
    color: var(--color-text-secondary);
    font-weight: var(--font-weight-medium);
  }

  .movie-list {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(200px, 1fr));
    gap: var(--spacing-lg);
  }

  // Mobile responsive
  @media (max-width: 768px) {
    main {
      padding: var(--spacing-lg);
    }

    .movie-list {
      grid-template-columns: repeat(auto-fill, minmax(150px, 1fr));
      gap: var(--spacing-md);
    }
  }

  @media (max-width: 480px) {
    .movie-list {
      grid-template-columns: 1fr 1fr;
    }
  }
</style>
