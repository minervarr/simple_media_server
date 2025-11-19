# Sokol Media Player - Cross-Platform Native Client

Cliente de reproductor de medios multiplataforma construido con las librerías Sokol, soportando **Windows**, **Linux** y **Android** desde un único código base en C.

## 🎯 Características

- ✅ **100% C puro** - Sin dependencias externas pesadas
- ✅ **Cross-platform** - Windows (D3D11), Linux (OpenGL 3.3), Android (OpenGL ES 3)
- ✅ **Ventana e Input unificados** - `sokol_app.h` abstrae las diferencias de plataforma
- ✅ **Audio streaming** - `sokol_audio.h` con backends nativos (WASAPI, ALSA, AAudio)
- ✅ **Gráficos 3D** - `sokol_gfx.h` wrapper unificado
- ✅ **Portable** - Mismo código fuente para PC y móvil

## 📦 Estructura del Proyecto

```
sokol-native/
├── src/
│   └── main.c              # Código principal (Windows/Linux/Android)
├── external/
│   └── sokol/              # Headers de Sokol
│       ├── sokol_app.h     # Ventana + Input
│       ├── sokol_audio.h   # Audio streaming
│       ├── sokol_gfx.h     # Gráficos 3D
│       ├── sokol_time.h    # Timing
│       └── sokol_log.h     # Logging
├── android/                # Proyecto Android Studio
│   ├── app/
│   │   ├── build.gradle
│   │   └── src/main/AndroidManifest.xml
│   ├── build.gradle
│   └── settings.gradle
├── CMakeLists.txt          # Build para PC
└── build.sh                # Script de compilación Linux/Mac
```

## 🚀 Compilación

### Linux / macOS

**Prerequisitos:**
```bash
# Ubuntu/Debian
sudo apt install build-essential cmake libx11-dev libxcursor-dev libxi-dev libasound2-dev

# Fedora
sudo dnf install gcc cmake libX11-devel libXcursor-devel libXi-devel alsa-lib-devel

# macOS
xcode-select --install
brew install cmake
```

**Compilar:**
```bash
cd sokol-native
./build.sh
```

**Ejecutar:**
```bash
./build/sokol_media_player
```

### Windows

**Prerequisitos:**
- Visual Studio 2019+ con C++ Desktop Development
- CMake 3.14+

**Compilar:**
```cmd
cd sokol-native
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

**Ejecutar:**
```cmd
build\Release\sokol_media_player.exe
```

### Android

**Prerequisitos:**
- Android Studio Arctic Fox (2020.3.1) o superior
- Android NDK 25+
- Android SDK API 24+ (Android 7.0)

**Pasos:**

1. Abrir Android Studio
2. `File` → `Open` → Seleccionar carpeta `sokol-native/android/`
3. Esperar a que Gradle sincronice
4. Conectar dispositivo Android o iniciar emulador
5. Click en `Run` (▶️)

**Build desde línea de comandos:**
```bash
cd sokol-native/android
./gradlew assembleRelease
# APK estará en: app/build/outputs/apk/release/app-release.apk
```

## 🎮 Controles

### PC (Mouse/Teclado)
- **Click izquierdo**: Interactuar
- **ESC**: Salir

### Android (Touch)
- **Tap**: Interactuar
- **Botón Back**: Salir

## 🔧 API de Sokol

### sokol_app.h - Ventana e Input

```c
// Entry point unificado
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

// Eventos unificados
void input_event(const sapp_event* e) {
    switch (e->type) {
        case SAPP_EVENTTYPE_KEY_DOWN:       // Teclado PC
        case SAPP_EVENTTYPE_TOUCHES_BEGAN:  // Touch móvil
        case SAPP_EVENTTYPE_MOUSE_DOWN:     // Mouse PC
            // Manejar input...
            break;
    }
}
```

### sokol_audio.h - Audio

```c
// Callback de audio (se ejecuta en thread separado)
void audio_callback(float* buffer, int num_frames, int num_channels) {
    for (int i = 0; i < num_frames; i++) {
        buffer[i*2 + 0] = left_sample;   // Canal izquierdo
        buffer[i*2 + 1] = right_sample;  // Canal derecho
    }
}

// Setup
saudio_setup(&(saudio_desc){
    .sample_rate = 44100,
    .num_channels = 2,
    .stream_cb = audio_callback,
});
```

### sokol_gfx.h - Gráficos

```c
// Setup (abstrae D3D11/OpenGL/GLES3)
sg_setup(&(sg_desc){
    .context = sapp_sgcontext(),
});

// Renderizar
sg_begin_default_pass(&pass_action, sapp_width(), sapp_height());
sg_apply_pipeline(pip);
sg_draw(0, 3, 1);
sg_end_pass();
sg_commit();
```

## 🔍 Backends por Plataforma

| Plataforma | Ventana | Gráficos | Audio | Input |
|------------|---------|----------|-------|-------|
| **Windows** | Win32 | D3D11 | WASAPI | Win32 |
| **Linux** | X11 | OpenGL 3.3 | ALSA | X11 |
| **Android** | ANativeWindow | OpenGL ES 3 | AAudio | Touch |
| **macOS** | Cocoa | Metal | CoreAudio | Cocoa |

## 📚 Arquitectura

```
┌─────────────────────────────┐
│   Tu código C (main.c)      │
├─────────────────────────────┤
│   sokol_app.h               │  ← Abstracción de ventana/input
│   sokol_audio.h             │  ← Abstracción de audio
│   sokol_gfx.h               │  ← Abstracción de gráficos
├─────────────────────────────┤
│   APIs nativas:             │
│   Win32 | X11 | ANativeWin  │  ← Por plataforma
│   D3D11 | GL  | GLES3       │
│   WASAPI| ALSA| AAudio      │
└─────────────────────────────┘
```

## 🎨 Próximos Pasos

### Reproducción de Video

Para añadir reproducción de video real, necesitarás:

1. **Decodificación de video**: FFmpeg, libvpx, o codec nativo
2. **Textura de video**: Subir frames decodificados a GPU
3. **Sincronización A/V**: Usar `sokol_time.h` para timing

Ejemplo básico:
```c
// Decodificar frame de video
AVFrame* frame = decode_next_frame();

// Subir a textura GPU
sg_update_image(video_texture, &(sg_image_data){
    .subimage[0][0] = {
        .ptr = frame->data[0],
        .size = frame->linesize[0] * frame->height
    }
});

// Renderizar quad con textura
sg_apply_pipeline(video_pipeline);
sg_apply_bindings(&video_bindings);
sg_draw(0, 6, 1);
```

### UI Nativa

Para añadir UI (botones, listas, etc.):

1. **Dear ImGui** + Sokol: https://github.com/floooh/sokol-samples
2. **Nuklear**: Lightweight immediate mode GUI
3. **Custom**: Renderizar primitivas con `sokol_gfx.h`

## 🐛 Troubleshooting

### Linux: "cannot find -lGL"
```bash
sudo apt install libgl1-mesa-dev
```

### Android: "Execution failed for task ':app:externalNativeBuildDebug'"
- Verifica que NDK esté instalado en Android Studio
- `File` → `Settings` → `Android SDK` → `SDK Tools` → `NDK`

### "sokol_app.h: No such file or directory"
```bash
# Descargar headers manualmente
cd sokol-native/external/sokol
curl -O https://raw.githubusercontent.com/floooh/sokol/master/sokol_app.h
curl -O https://raw.githubusercontent.com/floooh/sokol/master/sokol_audio.h
curl -O https://raw.githubusercontent.com/floooh/sokol/master/sokol_gfx.h
```

## 📖 Recursos

- **Sokol Headers**: https://github.com/floooh/sokol
- **Sokol Samples**: https://github.com/floooh/sokol-samples
- **Documentación**: Headers están auto-documentados (comentarios extensos)

## 🤝 Contribuir

Este es un proyecto base para portabilidad PC↔Android. Ideas:

- [ ] Integrar reproductor de video (FFmpeg)
- [ ] UI de biblioteca de medios
- [ ] Soporte para streaming HTTP
- [ ] Subtítulos
- [ ] Controles de reproducción táctil

## 📝 Licencia

Este proyecto usa las librerías Sokol que están bajo licencia MIT/zlib.

## 🙏 Créditos

- **Sokol**: https://github.com/floooh/sokol por Andre Weissflog
- Arquitectura basada en ejemplos de sokol-samples
