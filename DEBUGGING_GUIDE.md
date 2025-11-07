# Debugging Guide - Multi-Format Video System

## Overview

This guide explains how to debug issues with video playback, codec detection, and format switching using the comprehensive verbose logging now built into the system.

---

## Quick Start - Enable Verbose Logging

The system now has **verbose logging enabled by default**. You'll see detailed output in two places:

### 1. **Backend Logs** (Terminal)
Run the server and watch the terminal output:
```bash
cd backend/build
./media_server
```

### 2. **Frontend Logs** (Browser Console)
Open your browser's Developer Tools (F12) and check the Console tab.

---

## Understanding the Logs

### Backend Log Format

All backend logs are prefixed with tags:
- `[API]` - Video Info API endpoint
- `[HLS]` - HLS streaming endpoint
- `[VideoInfo]` - FFprobe codec detection
- `ERROR:` - Critical errors

### Frontend Log Format

All frontend logs are prefixed with tags:
- `[API]` - API fetch operations
- `[VideoPlayer]` - Video player component
- `[HLS.js]` - HLS.js library (if enabled)

---

## Common Issues & How to Debug

### Issue 1: "Loading video information..." Stuck Forever

**Symptoms:**
- Browser shows "Loading video information..." indefinitely
- Video never starts playing

**Debugging Steps:**

1. **Check Browser Console** (F12 → Console tab)

   Look for:
   ```
   [API] Fetching video info: /api/video/info/Movies/Example.mkv
   [API] Response status: XXX
   ```

   **If you see `404` or `500` status:**
   - Video file not found or permission denied
   - Check the backend logs for the full path

   **If you see `ERROR: Failed to fetch video info`:**
   - Backend video info API is failing
   - Check backend terminal for errors

2. **Check Backend Terminal**

   Look for:
   ```
   [API] ===== Video Info Request =====
   [API] Full URL: /api/video/info/...
   [API] Decoded path: ...
   [API] Full file path: ...
   [API] File exists, analyzing...
   ```

   **Common Errors:**

   a) **`ERROR: File does not exist`**
      ```
      [API] Full file path: /wrong/path/Movie.mkv
      [API] ERROR: File does not exist
      ```
      **Solution:** Check your `config.json` library_path setting

   b) **`ERROR: Failed to analyze video file`**
      ```
      [VideoInfo] ERROR: Failed to get ffprobe output
      ```
      **Solution:** Install ffprobe:
      ```bash
      # Ubuntu/Debian
      sudo apt-get install ffmpeg

      # macOS
      brew install ffmpeg

      # Verify installation
      ffprobe -version
      ```

   c) **`ffprobe timeout`**
      ```
      [VideoInfo] Running ffprobe...
      [VideoInfo] ffprobe completed in 10000ms
      ```
      **Solution:** File is corrupted or ffprobe is hanging
      - Try playing the file in VLC to verify it's valid
      - Check file permissions

3. **Frontend Fallback**

   If video info fails, the player should fallback to HLS mode:
   ```javascript
   [VideoPlayer] Failed to fetch video info, using default HLS mode
   ```

   If you DON'T see this, the error handling isn't working properly.

---

### Issue 2: Video Jumps/Skips During Playback

**Symptoms:**
- Video plays but jumps around
- Seeking doesn't work properly
- Some parts of video are skipped

**Debugging Steps:**

1. **Check HLS Stream Copy Status**

   In backend terminal, when video starts:
   ```
   [HLS] ===== HLS Playlist Request =====
   [HLS] Video path: Movies/Example.mkv
   [HLS] Not in cache, generating HLS stream...
   [HLS] ✓ Video is HLS compatible!
   [HLS] ✓ Using STREAM COPY (zero quality loss!)
   ```

   **OR**

   ```
   [HLS] ✗ Video requires transcoding for HLS
   [HLS]   Needs video transcode: yes
   [HLS]   Needs audio transcode: no
   ```

2. **Issue: Stream Copy but Still Jumping**

   **Cause:** Original video doesn't have regular keyframes

   **Solution:** Use Legacy Compatible format instead:
   - Click "⚙️ Format" button
   - Select "Legacy Compatible"
   - This will transcode with forced keyframes

3. **Issue: Transcoding but Still Jumping**

   **Cause:** Transcoding failed or ffmpeg error

   **Check Backend for:**
   ```
   Transcoding to H.264/AAC for: ...
   (look for ffmpeg error messages)
   ```

   **Common ffmpeg errors:**
   - `Unknown encoder 'libx264'` → Install ffmpeg with x264 support
   - `Invalid argument` → File is corrupted
   - `Protocol not found` → Path has special characters

---

### Issue 3: HLS Generation Takes Forever

**Symptoms:**
- "Loading hls..." message shows for minutes
- Backend terminal shows ffmpeg running but slow

**Debugging Steps:**

1. **Check Transcoding Speed**

   Look for:
   ```
   [HLS] Starting HLS generation...
   (wait time)
   [HLS] HLS generation complete!
   ```

   **Normal:** 5-30 seconds for first-time transcode
   **Slow:** > 1 minute for a short video

2. **Speed Up Transcoding**

   Edit `backend/main.cpp` line 68:
   ```cpp
   // Change from:
   << "-preset veryfast "

   // To:
   << "-preset ultrafast "
   ```

3. **Use Stream Copy Instead**

   If your video is already H.264/AAC, convert it to proper format:
   ```bash
   ffmpeg -i input.mkv -c:v copy -c:a copy -movflags +faststart output.mp4
   ```

   Then HLS will use stream copy (near-instant).

---

### Issue 4: Format Switching Doesn't Work

**Symptoms:**
- Click format button, video doesn't change
- Format selector doesn't appear

**Debugging Steps:**

1. **Check Browser Console**

   ```javascript
   [VideoPlayer] Loaded preference: { preferredMode: "hls", autoSelect: true }
   [VideoPlayer] Video info received: { ... }
   [VideoPlayer] Auto-selected mode: hls
   [VideoPlayer] Loading video with mode: hls
   ```

2. **Check Available Modes**

   In browser console:
   ```javascript
   [API] Video info fetched successfully: {
     videoStreams: 1,
     audioStreams: 1,
     playbackModes: 4
   }
   ```

   **If `playbackModes: 0`:**
   - Backend codec detection failed
   - Check backend logs for video analysis errors

3. **Verify Format Switch**

   When clicking a different format:
   ```javascript
   [VideoPlayer] Using preferred mode: original
   [VideoPlayer] Loading video with mode: original
   ```

---

## Detailed Log Examples

### Successful Video Playback (HLS Stream Copy)

**Backend:**
```
[API] ===== Video Info Request =====
[API] Full URL: /api/video/info/Movies/Example.mkv
[API] Extracted path: Movies/Example.mkv
[API] Decoded path: Movies/Example.mkv
[API] Full file path: /media/Movies/Example.mkv
[API] File exists, analyzing...
[VideoInfo] Analyzing video: /media/Movies/Example.mkv
[VideoInfo] Running ffprobe...
[VideoInfo] ffprobe completed in 234ms
[VideoInfo] ffprobe output size: 5432 bytes
[VideoInfo] Successfully parsed video info:
  - Video streams: 1
  - Audio streams: 1
  - Playback modes: 4
  - Codec: h264 1920x1080
[API] Analysis successful, sending response
[API] Response sent successfully
[API] ================================

[HLS] ===== HLS Playlist Request =====
[HLS] Video path: Movies/Example.mkv
[HLS] Full path: /media/Movies/Example.mkv
[HLS] Not in cache, generating HLS stream...
[VideoInfo] Analyzing video: /media/Movies/Example.mkv
[VideoInfo] Running ffprobe...
[VideoInfo] ffprobe completed in 198ms
[VideoInfo] Successfully parsed video info:
  - Codec: h264 1920x1080
[HLS] ✓ Video is HLS compatible!
[HLS] ✓ Using STREAM COPY (zero quality loss!)
[HLS] Starting HLS generation...
Using stream copy (no re-encoding) for: /media/Movies/Example.mkv
HLS generation complete: /tmp/media_server_hls/12345/playlist.m3u8
[HLS] HLS generation complete!
[HLS] Playlist served successfully
[HLS] ================================
```

**Frontend (Browser Console):**
```
[API] Fetching video info: /api/video/info/Movies%2FExample.mkv
[API] Response status: 200 OK
[API] Video info fetched successfully: {
  videoStreams: 1,
  audioStreams: 1,
  playbackModes: 4
}
[VideoPlayer] Starting video player for: Movies/Example.mkv
[VideoPlayer] Loaded preference: { preferredMode: "hls", autoSelect: true }
[VideoPlayer] Fetching video info from API...
[VideoPlayer] Video info received: {
  codec: "h264",
  resolution: "1920x1080",
  modes: ["original", "hls", "legacy", "download"]
}
[VideoPlayer] Auto-selected mode: hls
[VideoPlayer] Loading video with mode: hls
[VideoPlayer] Video loaded successfully
```

---

### Failed Video Info (Missing ffprobe)

**Backend:**
```
[API] ===== Video Info Request =====
[API] Full URL: /api/video/info/Movies/Example.mkv
[API] Full file path: /media/Movies/Example.mkv
[API] File exists, analyzing...
[VideoInfo] Analyzing video: /media/Movies/Example.mkv
[VideoInfo] Running ffprobe...
sh: 1: ffprobe: not found
[VideoInfo] ffprobe completed in 45ms
[VideoInfo] ERROR: Failed to get ffprobe output for: /media/Movies/Example.mkv
[API] ERROR: Failed to analyze video file
```

**Frontend:**
```
[API] Fetching video info: /api/video/info/Movies%2FExample.mkv
[API] Response status: 500 Internal Server Error
[API] Failed to fetch video info: {
  status: 500,
  statusText: "Internal Server Error",
  body: '{"error": "Failed to analyze video file. Check if ffprobe is installed."}'
}
[VideoPlayer] ERROR: Failed to fetch video info, using default HLS mode
```

**Solution:** Install ffmpeg/ffprobe

---

## Testing Your Setup

### 1. Test Video Info API

```bash
# Replace with your video path
curl http://localhost:8080/api/video/info/Movies/Example.mkv | jq .

# Should return JSON with:
# - format info
# - video_streams array
# - audio_streams array
# - playback_modes array
```

### 2. Test HLS Playlist

```bash
curl http://localhost:8080/hls/Movies/Example.mkv/playlist.m3u8

# Should return m3u8 playlist with segment references
```

### 3. Test Original Video Serving

```bash
curl -I http://localhost:8080/video/Movies/Example.mkv

# Should return:
# HTTP/1.1 200 OK
# Accept-Ranges: bytes
# Content-Length: ...
```

### 4. Test Browser Console

Open browser console and run:
```javascript
// Fetch video info
fetch('/api/video/info/Movies/Example.mkv')
  .then(r => r.json())
  .then(data => console.log('Video Info:', data));
```

---

## Performance Monitoring

### Timing Analysis

**Backend timing is logged:**
```
[VideoInfo] ffprobe completed in 234ms
```

**What's normal:**
- FFprobe: 50-500ms (depends on file size)
- HLS stream copy generation: 5-30 seconds
- HLS transcode generation: 30-300 seconds (depends on video length/resolution)

**What's slow:**
- FFprobe > 1000ms → Large file or slow disk
- HLS stream copy > 60s → Check disk I/O
- HLS transcode > 5 minutes → Lower preset or upgrade CPU

---

## Troubleshooting Checklist

When video won't play, check in order:

1. ☐ Browser console shows no errors
2. ☐ Backend terminal shows video info request
3. ☐ FFprobe command completes successfully
4. ☐ Video file exists at reported path
5. ☐ FFprobe is installed and in PATH
6. ☐ Video file is not corrupted (test in VLC)
7. ☐ HLS generation completes successfully
8. ☐ Frontend loads video URL without 404
9. ☐ No CORS errors in browser console
10. ☐ Video element src is set correctly

---

## Getting Help

If issues persist:

1. **Capture Backend Logs**
   ```bash
   cd backend/build
   ./media_server 2>&1 | tee server.log
   ```

2. **Capture Frontend Logs**
   - Open browser console (F12)
   - Right-click in console → "Save as..."

3. **Test Video File**
   ```bash
   # Test with ffprobe directly
   ffprobe -v quiet -print_format json -show_format -show_streams /path/to/video.mkv

   # Should output valid JSON
   ```

4. **Check File Permissions**
   ```bash
   ls -la /path/to/video.mkv
   # Should be readable by user running media_server
   ```

5. **Verify Network**
   ```bash
   # From another machine
   curl -I http://your-server-ip:8080/api/library
   ```

---

## Advanced Debugging

### Enable FFmpeg Verbose Output

Edit `backend/main.cpp` line 36:
```cpp
// Change from:
cmd << "ffmpeg -i \"" << videoPath.string() << "\" ";

// To:
cmd << "ffmpeg -v verbose -i \"" << videoPath.string() << "\" ";
```

Rebuild and run. You'll see detailed ffmpeg output.

### Monitor HLS Segment Generation

```bash
# Watch HLS cache directory
watch -n 1 ls -lh /tmp/media_server_hls/
```

### Analyze Network Traffic

Use browser DevTools Network tab:
1. Filter by "media" or "m3u8"
2. Check response times
3. Look for 404/500 errors
4. Verify Content-Type headers

---

## Summary

With verbose logging enabled, you should now be able to:

- ✅ See exactly what the video info API is doing
- ✅ Understand why stream copy is or isn't used
- ✅ Debug ffprobe failures
- ✅ Track HLS generation progress
- ✅ Monitor format switching behavior
- ✅ Identify network/path issues
- ✅ Verify codec compatibility

The logs tell you **everything** that's happening - use them!

---

## Quick Reference

| Problem | Log Location | What to Look For |
|---------|--------------|------------------|
| Video won't load | Browser console | `[API] Response status` |
| Stuck on "Loading..." | Both | API 404/500 errors |
| Playback jumps | Backend | `Using stream copy` or transcode errors |
| Slow start | Backend | HLS generation timing |
| Format switch fails | Browser console | `Loading video with mode:` |
| ffprobe errors | Backend | `[VideoInfo] ERROR:` |

Happy debugging! 🔍
