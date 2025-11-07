// Core data types matching backend JSON structures

export interface Profile {
  id: string;
  name: string;
  icon: string;
}

export interface Video {
  path: string;
  filename: string;
  episode?: number;
}

export interface Season {
  number: number;
  episodes: Video[];
}

export interface Series {
  name: string;
  displayName: string;
  seasons: Season[];
}

export interface Movie {
  name: string;
  path: string;
}

export interface Library {
  series: Series[];
  movies: Movie[];
}

export interface VideoPlayerInfo {
  path: string;
  title: string;
}
