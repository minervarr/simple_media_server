import { writable, derived, type Readable } from 'svelte/store';
import type { Profile } from '$lib/types';

// Profile data store
const profilesStore = writable<Profile[]>([]);
const currentProfileStore = writable<Profile | null>(null);
const showProfileSelectorStore = writable(true);
const profilesLoadingStore = writable(true);

// Watched videos per profile (stored in localStorage)
const watchedVideosStore = writable<Set<string>>(new Set());

// Profile store API (inspired by Apple's pattern)
export interface ProfileStore extends Readable<Profile[]> {
  load: () => Promise<void>;
  selectProfile: (profile: Profile) => void;
  showSelector: () => void;
}

export const profiles: ProfileStore = {
  subscribe: profilesStore.subscribe,

  load: async () => {
    try {
      const response = await fetch('/api/profiles');
      if (!response.ok) throw new Error('Failed to fetch profiles');

      const data = await response.json();
      profilesStore.set(data);
      profilesLoadingStore.set(false);

      // Try to restore last selected profile from localStorage
      const lastProfileId = localStorage.getItem('current_profile_id');
      if (lastProfileId) {
        const profile = data.find((p: Profile) => p.id === lastProfileId);
        if (profile) {
          currentProfileStore.set(profile);
          showProfileSelectorStore.set(false);
          loadWatchedVideos(profile.id);
          return;
        }
      }

      // No saved profile, show selector
      showProfileSelectorStore.set(true);
    } catch (error) {
      console.error('Failed to load profiles:', error);
      profilesLoadingStore.set(false);
    }
  },

  selectProfile: (profile: Profile) => {
    // Save watched videos for current profile before switching
    const currentProfile = getCurrentProfile();
    if (currentProfile) {
      saveWatchedVideos(currentProfile.id);
    }

    // Switch to new profile
    currentProfileStore.set(profile);
    localStorage.setItem('current_profile_id', profile.id);

    // Load watched videos for new profile
    loadWatchedVideos(profile.id);

    // Hide selector
    showProfileSelectorStore.set(false);
  },

  showSelector: () => {
    showProfileSelectorStore.set(true);
  },
};

export const currentProfile = {
  subscribe: currentProfileStore.subscribe,
};

export const showProfileSelector = {
  subscribe: showProfileSelectorStore.subscribe,
};

export const profilesLoading = {
  subscribe: profilesLoadingStore.subscribe,
};

export const watchedVideos = {
  subscribe: watchedVideosStore.subscribe,
  toggle: (videoPath: string) => {
    watchedVideosStore.update((videos) => {
      const newSet = new Set(videos);
      if (newSet.has(videoPath)) {
        newSet.delete(videoPath);
      } else {
        newSet.add(videoPath);
      }

      // Save to localStorage
      const currentProfile = getCurrentProfile();
      if (currentProfile) {
        saveWatchedVideosSet(currentProfile.id, newSet);
      }

      return newSet;
    });
  },
  markAsWatched: (videoPath: string) => {
    watchedVideosStore.update((videos) => {
      const newSet = new Set(videos);
      newSet.add(videoPath);

      const currentProfile = getCurrentProfile();
      if (currentProfile) {
        saveWatchedVideosSet(currentProfile.id, newSet);
      }

      return newSet;
    });
  },
};

// Helper functions
function loadWatchedVideos(profileId: string) {
  const storageKey = `watched_videos_${profileId}`;
  const stored = localStorage.getItem(storageKey);
  if (stored) {
    try {
      const array = JSON.parse(stored);
      watchedVideosStore.set(new Set(array));
    } catch {
      watchedVideosStore.set(new Set());
    }
  } else {
    watchedVideosStore.set(new Set());
  }
}

function saveWatchedVideos(profileId: string) {
  let videos: Set<string>;
  watchedVideosStore.subscribe((v) => videos = v)();
  saveWatchedVideosSet(profileId, videos!);
}

function saveWatchedVideosSet(profileId: string, videos: Set<string>) {
  const storageKey = `watched_videos_${profileId}`;
  localStorage.setItem(storageKey, JSON.stringify(Array.from(videos)));
}

function getCurrentProfile(): Profile | null {
  let profile: Profile | null = null;
  currentProfileStore.subscribe((p) => profile = p)();
  return profile;
}
