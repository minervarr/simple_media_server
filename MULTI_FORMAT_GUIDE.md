# Multi-Format Video Codec Compatibility Guide

## Overview

The Simple Media Server now supports **intelligent multi-format video serving** with NO quality degradation. You can choose between different codecs and containers to ensure compatibility with all your devices, from modern computers to legacy devices.

## Key Features

### 1. **Zero Quality Degradation**
- Original files can be served directly without any transcoding
- Smart HLS uses stream copy when source is already H.264/AAC (no re-encoding)
- Only transcode when explicitly choosing a compatibility format

### 2. **Multiple Format Options**
Every video offers up to 4 playback modes:

#### **Original Quality** (Best Quality)
- **No transcoding** - Your original file served as-is
- Preserves all codecs: HEVC/H.265, VP9, AV1, etc.
- Best video quality, highest bitrate
- May not work on older devices
- ✅ **Recommended if your device supports the codec**

#### **HLS Streaming** (Web Compatible)
- Automatic smart transcoding detection
- **Stream copy** if source is H.264/H.265 + AAC (no quality loss!)
- Re-encodes only when necessary
- Smooth seeking, adaptive playback
- Works in all modern web browsers
- ✅ **Recommended for web playback**

#### **Legacy Compatible** (Universal)
- Transcodes to H.264 Baseline + AAC in MP4
- Works on ALL devices:
  - Old Smart TVs
  - Budget Android phones
  - Legacy media players
  - Devices without hardware decoders
- Some quality loss during transcoding
- ✅ **Recommended for maximum compatibility**

#### **Direct Download/Link**
- Direct link to original file
- For use with external native video players
- Perfect for downloading to mobile devices
- VLC, MPC-HC, MPV, etc.

### 3. **Format Selector UI**
Click the **"⚙️ Format"** button in the video player to:
- See codec information (resolution, bitrate, audio channels)
- Choose between available formats
- View which formats require transcoding
- See file size and technical details

### 4. **Smart Auto-Selection**
The player automatically selects the best format based on:
- Your device capabilities (HEVC support, etc.)
- Browser compatibility
- Saved preferences

### 5. **Format Preferences Storage**
Your format choice is saved and automatically used for future videos.

---

## Backend API Endpoints

### **Video Information Endpoint**
```
GET /api/video/info/{video_path}
```
Returns detailed codec and format information:
- Video streams (codec, resolution, bitrate, bit depth)
- Audio streams (codec, channels, sample rate)
- Subtitle streams
- Compatibility flags
- Available playback modes

**Example Response:**
```json
{
  "file_path": "Movies/Example.mkv",
  "format": {
    "name": "matroska,webm",
    "long_name": "Matroska / WebM",
    "duration": 7265.5,
    "size": 15728640000,
    "bitrate": 17324567
  },
  "video_streams": [{
    "codec_name": "hevc",
    "codec_long_name": "H.265 / HEVC",
    "profile": "Main 10",
    "width": 3840,
    "height": 2160,
    "fps": 23.976,
    "bitrate": 15000000,
    "bit_depth": 10
  }],
  "audio_streams": [{
    "codec_name": "aac",
    "channels": 6,
    "channel_layout": "5.1",
    "bitrate": 256000
  }],
  "compatibility": {
    "is_hls_compatible": true,
    "needs_video_transcode": false,
    "needs_audio_transcode": false,
    "is_legacy_compatible": false
  },
  "playback_modes": [
    {
      "id": "original",
      "name": "Original Quality",
      "description": "hevc 3840x2160 10-bit + aac (No transcoding, best quality)",
      "requires_transcoding": false,
      "format_type": "original"
    },
    {
      "id": "hls",
      "name": "HLS Streaming",
      "description": "Stream copy (no re-encoding) - Best quality with seeking support",
      "requires_transcoding": false,
      "format_type": "hls"
    },
    {
      "id": "legacy",
      "name": "Legacy Compatible",
      "description": "Transcode to H.264 Baseline + AAC MP4 - Compatible with all devices",
      "requires_transcoding": true,
      "format_type": "legacy"
    }
  ]
}
```

### **Video Serving Endpoints**

#### Original Quality
```
GET /video/{video_path}
```
- Serves original file without modification
- Supports HTTP range requests (seeking)
- Zero transcoding

#### HLS Streaming (Smart Transcoding)
```
GET /hls/{video_path}/playlist.m3u8
GET /hls/{video_path}/segment{N}.ts
```
- Automatically detects if stream copy is possible
- Uses stream copy for H.264/H.265 + AAC (no re-encoding)
- Transcodes only when necessary
- 4-second segments for smooth seeking

#### Legacy Compatible MP4
```
GET /legacy/{video_path}
```
- Generates H.264 Baseline + AAC MP4 on demand
- Caches transcoded file for future use
- Serves original if already legacy-compatible
- Supports HTTP range requests

---

## How It Works

### Smart HLS Transcoding Logic

```cpp
// 1. Analyze video with ffprobe
auto videoInfo = VideoInfoAnalyzer::analyze(videoPath);

// 2. Check if HLS-compatible (H.264/H.265 + AAC)
if (videoInfo->is_hls_compatible) {
    // Use stream copy - NO RE-ENCODING!
    ffmpeg -i input.mkv -c:v copy -c:a copy -f hls output.m3u8
} else {
    // Transcode for compatibility
    ffmpeg -i input.mkv -c:v libx264 -crf 23 -c:a aac -f hls output.m3u8
}
```

### Legacy MP4 Generation

```cpp
// Generate maximum compatibility MP4
ffmpeg -i input.mkv \
    -c:v libx264 \
    -profile:v baseline \  // Most compatible H.264 profile
    -level 3.1 \           // Wide device support
    -pix_fmt yuv420p \     // 8-bit color
    -c:a aac \
    -ac 2 \                // Stereo (legacy devices)
    -movflags +faststart \ // Web streaming
    output.mp4
```

---

## Device Compatibility

### Modern Devices
**What they support:**
- H.264, H.265/HEVC (Apple Silicon, recent GPUs)
- VP9 (Chrome, Firefox)
- AV1 (very recent browsers/hardware)
- AAC, MP3, Opus audio

**Recommended format:** Original Quality or HLS

### Legacy Devices
**What they typically support:**
- H.264 Baseline/Main profile only
- AAC audio (stereo)
- MP4 container

**Recommended format:** Legacy Compatible

### Examples by Device Type

| Device Type | Recommended Format | Notes |
|------------|-------------------|-------|
| Modern PC/Mac | Original Quality | Best quality, no transcoding |
| iPhone/iPad | Original Quality or HLS | Native HEVC support |
| Android (2020+) | Original Quality or HLS | Most support HEVC |
| Old Smart TV | Legacy Compatible | H.264 Baseline only |
| Budget Android | Legacy Compatible | Limited codec support |
| Game Console | HLS or Legacy | Varies by console |
| Roku/Fire TV | HLS | Good H.264 support |

---

## Frontend Implementation

### Detecting Device Capabilities

```typescript
const capabilities = detectDeviceCapabilities();

// Returns:
{
  supportsHEVC: boolean,  // Can play H.265
  supportsVP9: boolean,   // Can play VP9
  supportsAV1: boolean,   // Can play AV1
  supportsH264: boolean,  // Can play H.264
  supportsHLS: boolean    // Native HLS support
}
```

### Format Selection

Users can manually select format via the UI:
1. Click "⚙️ Format" button
2. See available options with codec info
3. Select preferred format
4. Video switches seamlessly (maintains playback position)

### Automatic Format Selection

```typescript
// Load saved preference
const preference = loadFormatPreference();

if (preference.autoSelect) {
  // Auto-select based on device capabilities
  const videoCodec = videoInfo.video_streams[0].codec_name;
  selectedMode = getRecommendedMode(deviceCapabilities, videoCodec);
}
```

---

## Performance Considerations

### Caching
- **HLS segments:** Cached in `/tmp/media_server_hls/`
- **Legacy MP4 files:** Cached in `/tmp/media_server_legacy/`
- Once generated, cached files are reused (no re-transcoding)

### Transcoding Speed
- **HLS transcode:** Uses `veryfast` preset (real-time capable)
- **Legacy transcode:** Uses `medium` preset (better quality, slower)
- **Stream copy:** Near-instant (no encoding, just remuxing)

### Bandwidth Usage
- **Original:** Full source bitrate
- **HLS:** Same as source (stream copy) or transcoded bitrate
- **Legacy:** Moderate bitrate (CRF 23 ≈ 3-8 Mbps for 1080p)

---

## Troubleshooting

### "Video won't play on my device"
**Solution:** Try Legacy Compatible format

### "Video quality is poor"
**Solution:** Use Original Quality format if your device supports it

### "Seeking is slow/broken"
**Solution:** Use HLS Streaming format (has keyframes for seeking)

### "First playback is slow"
**Solution:** First time requires transcoding; subsequent plays use cache

### "How do I check codec compatibility?"
**Solution:** Click "⚙️ Format" button to see detailed codec information

---

## Technical Details

### Supported Input Codecs
**Video:** H.264, H.265/HEVC, VP9, AV1, MPEG-2, MPEG-4, VC-1, and more
**Audio:** AAC, MP3, AC3, DTS, FLAC, Opus, Vorbis, and more
**Containers:** MP4, MKV, AVI, MOV, WebM, and more

### Output Formats

| Format | Video Codec | Audio Codec | Container | Profile |
|--------|-------------|-------------|-----------|---------|
| Original | (unchanged) | (unchanged) | (original) | N/A |
| HLS | H.264 or copy | AAC or copy | MPEGTS | High/Main |
| Legacy | H.264 | AAC | MP4 | Baseline |

### FFmpeg Command Examples

**Stream Copy (HLS):**
```bash
ffmpeg -i input.mkv -c:v copy -c:a copy \
  -hls_time 4 -hls_list_size 0 \
  -hls_flags independent_segments+split_by_time \
  output.m3u8
```

**Transcode (HLS):**
```bash
ffmpeg -i input.mkv \
  -c:v libx264 -preset veryfast -crf 23 \
  -g 48 -sc_threshold 0 \
  -c:a aac -b:a 128k \
  -hls_time 4 -hls_list_size 0 \
  output.m3u8
```

**Legacy Compatible:**
```bash
ffmpeg -i input.mkv \
  -c:v libx264 -profile:v baseline -level 3.1 \
  -pix_fmt yuv420p -preset medium -crf 23 \
  -c:a aac -b:a 128k -ac 2 \
  -movflags +faststart \
  output.mp4
```

---

## Summary

✅ **Zero quality loss** - Original files served as-is
✅ **Smart transcoding** - Stream copy when possible
✅ **Universal compatibility** - Works on all devices
✅ **User choice** - Manual format selection
✅ **Auto-detection** - Smart format selection
✅ **Performance** - Caching prevents re-transcoding
✅ **Transparency** - See exactly what's happening with each format

Choose the format that best fits your needs and device capabilities!
