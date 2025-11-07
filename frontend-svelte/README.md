# Media Server Frontend (Svelte)

Professional Svelte frontend for the Simple Media Server, inspired by modern web applications.

## Features

- **TypeScript** - Full type safety
- **Svelte Stores** - Native state management
- **SCSS** - Professional styling with design tokens
- **Component Architecture** - Modular, reusable components
- **HLS Video Player** - Streaming video with hls.js
- **Mobile Optimized** - Android clipboard fallback, responsive design
- **Profile Management** - Multi-user support with watched status tracking
- **AMOLED Black Theme** - Battery-friendly dark theme

## Development

```bash
# Install dependencies
npm install

# Run development server
npm run dev

# Build for production
npm run build

# Preview production build
npm run preview
```

## Project Structure

```
src/
├── lib/
│   ├── components/      # Reusable Svelte components
│   ├── stores/          # Svelte stores for state management
│   ├── utils/           # Utility functions
│   └── types/           # TypeScript type definitions
├── assets/
│   └── styles/          # Global SCSS styles
├── App.svelte           # Main application component
└── main.ts              # Entry point
```

## Architecture

This frontend uses:
- **Svelte Stores** for state management (no Redux needed!)
- **Component-based architecture** for maintainability
- **TypeScript** for type safety
- **SCSS with design tokens** for consistent styling
- **HLS.js** for video streaming
- **Vite** for fast builds and hot module replacement

## Backend Integration

The frontend connects to the C++ backend via:
- `/api/profiles` - Get user profiles
- `/api/library` - Get media library
- `/video/:path` - Direct video access
- `/hls/:path/playlist.m3u8` - HLS streaming

Development server proxies these routes to `http://localhost:8080`.
