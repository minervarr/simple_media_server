import { writable, derived } from 'svelte/store';
import type { Library, Series, Movie } from '$lib/types';

// Library data store
const libraryStore = writable<Library | null>(null);
const loadingStore = writable(true);
const errorStore = writable<string | null>(null);
const searchQueryStore = writable('');

// Expanded state for accordion UI
const expandedSeriesStore = writable<Set<string>>(new Set());
const expandedSeasonsStore = writable<Set<string>>(new Set());

// Load library data
export const library = {
  subscribe: libraryStore.subscribe,
  load: async () => {
    try {
      loadingStore.set(true);
      const response = await fetch('/api/library');
      if (!response.ok) throw new Error('Failed to fetch library');

      const data = await response.json();
      libraryStore.set(data);
      loadingStore.set(false);
    } catch (error) {
      console.error('Failed to load library:', error);
      errorStore.set(error instanceof Error ? error.message : 'Unknown error');
      loadingStore.set(false);
    }
  },
};

export const loading = {
  subscribe: loadingStore.subscribe,
};

export const error = {
  subscribe: errorStore.subscribe,
};

export const searchQuery = {
  subscribe: searchQueryStore.subscribe,
  set: (query: string) => searchQueryStore.set(query.toLowerCase()),
};

// Filtered library based on search query
export const filteredLibrary = derived(
  [libraryStore, searchQueryStore],
  ([$library, $searchQuery]) => {
    if (!$library) return null;
    if (!$searchQuery) return $library;

    const filteredSeries = $library.series.filter((series) =>
      series.displayName.toLowerCase().includes($searchQuery) ||
      series.name.toLowerCase().includes($searchQuery)
    );

    const filteredMovies = $library.movies.filter((movie) =>
      movie.name.toLowerCase().includes($searchQuery)
    );

    return {
      series: filteredSeries,
      movies: filteredMovies,
    };
  }
);

// Expansion controls
export const expandedSeries = {
  subscribe: expandedSeriesStore.subscribe,
  toggle: (seriesName: string) => {
    expandedSeriesStore.update((set) => {
      const newSet = new Set(set);
      if (newSet.has(seriesName)) {
        newSet.delete(seriesName);
      } else {
        newSet.add(seriesName);
      }
      return newSet;
    });
  },
};

export const expandedSeasons = {
  subscribe: expandedSeasonsStore.subscribe,
  toggle: (seriesName: string, seasonNumber: number) => {
    const key = `${seriesName}_${seasonNumber}`;
    expandedSeasonsStore.update((set) => {
      const newSet = new Set(set);
      if (newSet.has(key)) {
        newSet.delete(key);
      } else {
        newSet.add(key);
      }
      return newSet;
    });
  },
};
