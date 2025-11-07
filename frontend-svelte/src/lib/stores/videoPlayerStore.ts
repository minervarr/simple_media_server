import { writable } from 'svelte/store';
import type { VideoPlayerInfo } from '$lib/types';

const videoPlayerStore = writable<VideoPlayerInfo | null>(null);
const copiedVideoStore = writable<string | null>(null);

export const videoPlayer = {
  subscribe: videoPlayerStore.subscribe,
  play: (path: string, title: string) => {
    videoPlayerStore.set({ path, title });
  },
  close: () => {
    videoPlayerStore.set(null);
  },
};

export const copiedVideo = {
  subscribe: copiedVideoStore.subscribe,
  set: (path: string | null) => {
    copiedVideoStore.set(path);
  },
};
