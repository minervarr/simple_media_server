# Arquitectura del Proyecto Sokol Media Player

## 📐 Visión General

Este proyecto implementa un cliente de reproductor de medios multiplataforma usando las librerías **Sokol** en C puro, capaz de ejecutarse en **Windows**, **Linux** y **Android** desde un único código base.

## 🏗️ Arquitectura de Capas

```
┌──────────────────────────────────────────────────────────┐
│                     CAPA DE APLICACIÓN                    │
│   - Video Player UI                                       │
│   - Controles de reproducción                            │
│   - Biblioteca de medios                                 │
│   - HTTP Client (conexión con backend)                   │
└────────────────────┬─────────────────────────────────────┘
                     │
┌────────────────────▼─────────────────────────────────────┐
│                  CAPA DE ABSTRACCIÓN SOKOL               │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐   │
│  │ sokol_app.h  │  │sokol_audio.h │  │ sokol_gfx.h  │   │
│  │ Ventana      │  │ Audio        │  │ Gráficos     │   │
│  │ Input        │  │ Streaming    │  │ 3D           │   │
│  └──────────────┘  └──────────────┘  └──────────────┘   │
│  ┌──────────────┐  ┌──────────────┐                      │
│  │sokol_time.h  │  │ sokol_log.h  │                      │
│  │ Timing       │  │ Logging      │                      │
│  └──────────────┘  └──────────────┘                      │
└────────────────────┬─────────────────────────────────────┘
                     │
┌────────────────────▼─────────────────────────────────────┐
│              CAPA DE PLATAFORMA NATIVA                   │
│  ┌──────────────────────────────────────────────────┐   │
│  │           Windows (Win32 + D3D11)                │   │
│  │  - Win32 API para ventanas                       │   │
│  │  - Direct3D 11 para renderizado                  │   │
│  │  - WASAPI para audio                             │   │
│  └──────────────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────────┐   │
│  │         Linux (X11 + OpenGL 3.3)                 │   │
│  │  - X11 para ventanas                             │   │
│  │  - OpenGL 3.3 Core para renderizado              │   │
│  │  - ALSA para audio                               │   │
│  └──────────────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────────┐   │
│  │      Android (ANativeWindow + GLES3)             │   │
│  │  - ANativeWindow para ventanas                   │   │
│  │  - OpenGL ES 3 para renderizado                  │   │
│  │  - AAudio para audio                             │   │
│  └──────────────────────────────────────────────────┘   │
└──────────────────────────────────────────────────────────┘
```

## 🎨 Componentes Principales

### 1. sokol_app.h - Gestión de Ventanas e Input

**Propósito:** Abstrae la creación de ventanas y el manejo de eventos de entrada.

**Características:**
- Entry point unificado: `sokol_main()`
- Sistema de eventos común para todas las plataformas
- Manejo de teclado, mouse y touch
- Gestión del ciclo de vida de la aplicación

**Implementación por Plataforma:**

| Plataforma | Backend | Características Especiales |
|------------|---------|----------------------------|
| Windows | Win32 | Ventanas nativas, mensajes Win32 |
| Linux | X11 | Integración con Window Manager |
| Android | `android_native_app_glue` | Soporte para touch, teclado virtual |

**Ejemplo de Uso:**
```c
sapp_desc sokol_main(int argc, char* argv[]) {
    return (sapp_desc){
        .init_cb = init,
        .frame_cb = frame,
        .event_cb = input_event,
        .width = 800,
        .height = 600,
        .window_title = "Mi App",
    };
}
```

### 2. sokol_gfx.h - Renderizado Gráfico

**Propósito:** Wrapper unificado sobre diferentes APIs gráficas.

**Backends Soportados:**
- **D3D11** (Windows)
- **OpenGL 3.3 Core** (Linux, macOS)
- **OpenGL ES 3** (Android, iOS)
- **Metal** (macOS, iOS)
- **WebGPU** (Web)

**Pipeline de Renderizado:**
1. Crear recursos (buffers, shaders, texturas)
2. Configurar pipeline state
3. Aplicar bindings (buffers + texturas)
4. Dibujar

**Ejemplo de Renderizado de Video:**
```c
// Crear texturas para YUV420 (formato común de video)
sg_image y_tex = sg_make_image(&(sg_image_desc){
    .width = video_width,
    .height = video_height,
    .pixel_format = SG_PIXELFORMAT_R8,
    .usage = SG_USAGE_STREAM,
});

// En cada frame:
sg_update_image(y_tex, &(sg_image_data){
    .subimage[0][0] = { .ptr = y_data, .size = y_size }
});
```

### 3. sokol_audio.h - Audio Streaming

**Propósito:** Sistema de audio streaming de baja latencia.

**Backends por Plataforma:**
- **WASAPI** (Windows)
- **ALSA** (Linux)
- **AAudio** (Android 8.0+) / OpenSLES (fallback)
- **CoreAudio** (macOS, iOS)
- **WebAudio** (Web)

**Modelos de Uso:**

**A) Callback (recomendado para video):**
```c
void audio_callback(float* buffer, int num_frames, int num_channels) {
    // Llenar buffer con samples de audio decodificado
    memcpy(buffer, decoded_audio, num_frames * num_channels * sizeof(float));
}

saudio_setup(&(saudio_desc){
    .sample_rate = 48000,
    .num_channels = 2,
    .stream_cb = audio_callback,
});
```

**B) Push (para generación en main loop):**
```c
saudio_setup(&(saudio_desc){
    .sample_rate = 44100,
    .num_channels = 2,
});

// En loop:
int frames_needed = saudio_expect();
saudio_push(audio_data, frames_needed);
```

### 4. sokol_time.h - Timing Preciso

**Propósito:** Medición de tiempo de alta resolución para sincronización.

**Uso para Reproducción de Video:**
```c
stm_setup();

uint64_t last_frame_time = stm_now();

// En cada frame:
uint64_t now = stm_now();
double dt = stm_sec(stm_diff(now, last_frame_time));

// Calcular si es momento de mostrar siguiente frame
if (dt >= (1.0 / video_fps)) {
    decode_and_show_next_frame();
    last_frame_time = now;
}
```

## 🎬 Flujo de Reproducción de Video

### Pipeline Completo

```
┌─────────────────────────────────────────────────────────┐
│ 1. CARGA DE VIDEO                                       │
│    - HTTP streaming desde backend                       │
│    - Lectura de archivo local                           │
└────────────────────┬────────────────────────────────────┘
                     │
┌────────────────────▼────────────────────────────────────┐
│ 2. DEMUXING (FFmpeg avformat)                           │
│    - Separar streams de video, audio, subtítulos        │
│    - Extraer timestamps (PTS/DTS)                       │
└────────────────────┬────────────────────────────────────┘
                     │
         ┌───────────┴──────────┐
         │                      │
┌────────▼─────────┐  ┌─────────▼────────┐
│ 3A. VIDEO DECODE │  │ 3B. AUDIO DECODE │
│ (avcodec)        │  │ (avcodec)        │
│ - H.264/HEVC     │  │ - AAC/MP3/AC3    │
│ - VP9/AV1        │  │ - Resampling     │
└────────┬─────────┘  └─────────┬────────┘
         │                      │
┌────────▼─────────┐  ┌─────────▼────────┐
│ 4A. FRAME BUFFER │  │ 4B. AUDIO BUFFER │
│ - Ring buffer    │  │ - Ring buffer    │
│ - YUV frames     │  │ - Float samples  │
└────────┬─────────┘  └─────────┬────────┘
         │                      │
┌────────▼─────────┐  ┌─────────▼────────┐
│ 5A. GPU UPLOAD   │  │ 5B. AUDIO OUTPUT │
│ (sokol_gfx)      │  │ (sokol_audio)    │
│ - Update texture │  │ - Stream callback│
└────────┬─────────┘  └──────────────────┘
         │
┌────────▼─────────┐
│ 6. RENDER        │
│ - YUV→RGB shader │
│ - Present frame  │
└──────────────────┘
```

## 🔌 Integración con Backend

### Arquitectura Cliente-Servidor

```
┌──────────────────────────────────────────┐
│   Backend C++ (media_server)             │
│   Puerto: 8080                           │
│   ┌────────────────────────────────┐     │
│   │ API REST                       │     │
│   │ GET /api/library               │     │
│   │ GET /api/profiles              │     │
│   └────────────────────────────────┘     │
│   ┌────────────────────────────────┐     │
│   │ Video Streaming                │     │
│   │ GET /video/{path}              │     │
│   │ GET /hls/{path}/playlist.m3u8  │     │
│   │ GET /legacy/{path}             │     │
│   └────────────────────────────────┘     │
└──────────────────┬───────────────────────┘
                   │ HTTP/JSON
                   │ Video Stream
┌──────────────────▼───────────────────────┐
│   Cliente Sokol (sokol_media_player)     │
│   Plataformas: Windows, Linux, Android   │
│   ┌────────────────────────────────┐     │
│   │ HTTP Client (libcurl)          │     │
│   │ - Fetch library                │     │
│   │ - Stream video                 │     │
│   └────────────────────────────────┘     │
│   ┌────────────────────────────────┐     │
│   │ Video Decoder (FFmpeg)         │     │
│   │ - H.264/HEVC decode            │     │
│   │ - Audio decode                 │     │
│   └────────────────────────────────┘     │
│   ┌────────────────────────────────┐     │
│   │ Playback Engine                │     │
│   │ - A/V sync                     │     │
│   │ - Buffering                    │     │
│   │ - Controls                     │     │
│   └────────────────────────────────┘     │
│   ┌────────────────────────────────┐     │
│   │ Sokol Rendering                │     │
│   │ - sokol_app (ventana)          │     │
│   │ - sokol_gfx (video frame)      │     │
│   │ - sokol_audio (sonido)         │     │
│   └────────────────────────────────┘     │
└──────────────────────────────────────────┘
```

### Flujo de Datos

1. **Cliente inicia**: Conecta a `http://servidor:8080`
2. **Fetch biblioteca**: `GET /api/library` → JSON con lista de videos
3. **Usuario selecciona video**:
   - Opción A: Streaming directo → `GET /video/{path}`
   - Opción B: HLS adaptativo → `GET /hls/{path}/playlist.m3u8`
4. **Decodificación**: FFmpeg procesa el stream
5. **Reproducción**: Sokol renderiza frames y reproduce audio

## 🔐 Consideraciones de Seguridad

### Red Local
- El backend no tiene autenticación por defecto
- Solo usar en red local confiable
- Para acceso remoto: usar VPN o SSH tunnel

### Android Permisos
```xml
<uses-permission android:name="android.permission.INTERNET" />
<uses-permission android:name="android.permission.READ_EXTERNAL_STORAGE" />
```

## ⚙️ Build System

### PC (CMake)

```cmake
# Windows: D3D11
# Linux: OpenGL 3.3
# Detecta plataforma automáticamente

cmake_minimum_required(VERSION 3.14)
project(sokol_media_player C)

# Sokol se incluye como headers
include_directories(external/sokol)

add_executable(sokol_media_player src/main.c)
target_link_libraries(sokol_media_player ${PLATFORM_LIBS})
```

### Android (Gradle + CMake)

```gradle
android {
    externalNativeBuild {
        cmake {
            path file('../../CMakeLists.txt')
        }
    }

    defaultConfig {
        ndk {
            abiFilters 'arm64-v8a', 'armeabi-v7a'
        }
    }
}
```

## 📊 Performance

### Benchmarks Típicos

| Operación | Windows | Linux | Android |
|-----------|---------|-------|---------|
| Startup | < 50ms | < 50ms | < 200ms |
| Frame decode (1080p H.264) | 5-10ms | 5-10ms | 10-20ms |
| GPU upload | 1-2ms | 1-2ms | 2-5ms |
| Render | 1ms | 1ms | 2ms |
| Audio latency | 10-20ms | 20-30ms | 20-40ms |

### Optimizaciones

1. **Decodificación en thread separado**: Evitar bloquear el render loop
2. **Ring buffers**: Para video y audio
3. **Hardware decode**: DXVA2 (Windows), VAAPI (Linux), MediaCodec (Android)
4. **Doble/triple buffering**: Para frames de video

## 🔮 Roadmap

### Fase 1: Core Funcional ✅
- [x] Setup de Sokol
- [x] Renderizado básico
- [x] Input handling
- [x] Build system Android

### Fase 2: Video Player
- [ ] Integrar FFmpeg
- [ ] Decodificación de video
- [ ] Reproducción de audio
- [ ] Sincronización A/V

### Fase 3: UI y Controles
- [ ] Interfaz de controles (play/pause/seek)
- [ ] Lista de videos
- [ ] Subtítulos
- [ ] Pantalla completa

### Fase 4: Optimización
- [ ] Hardware decode
- [ ] Buffering inteligente
- [ ] Manejo de errores de red

### Fase 5: Características Avanzadas
- [ ] Soporte para HLS
- [ ] Picture-in-Picture (Android)
- [ ] Chromecast
- [ ] Sincronización de progreso

## 📚 Referencias

- **Sokol Headers**: https://github.com/floooh/sokol
- **FFmpeg**: https://ffmpeg.org/documentation.html
- **Android NDK**: https://developer.android.com/ndk/guides
- **OpenGL ES**: https://www.khronos.org/opengles/
