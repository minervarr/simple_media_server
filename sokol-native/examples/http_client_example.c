/*
 * HTTP Client Example - Conectar con el backend del media server
 *
 * Este ejemplo muestra cómo el cliente nativo Sokol puede
 * conectarse al backend C++ del media server para obtener
 * la biblioteca de videos y reproducirlos.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// En un proyecto real, usar libcurl o similar para HTTP
// Este es un ejemplo conceptual de la arquitectura

typedef struct {
    char* title;
    char* path;
    int season;
    int episode;
} Video;

typedef struct {
    Video* videos;
    int count;
} VideoLibrary;

// Ejemplo de cómo se conectaría al servidor
VideoLibrary* fetch_library(const char* server_url) {
    // En la práctica:
    // 1. Hacer petición HTTP GET a http://server:8080/api/library
    // 2. Parsear JSON (usar cJSON, json-parser, o similar)
    // 3. Llenar estructura VideoLibrary

    printf("Fetching library from %s/api/library\n", server_url);

    // Simulación
    VideoLibrary* lib = malloc(sizeof(VideoLibrary));
    lib->count = 3;
    lib->videos = malloc(sizeof(Video) * lib->count);

    lib->videos[0] = (Video){
        .title = strdup("Series 1 - S01E01"),
        .path = strdup("/series1/S01E01.mkv"),
        .season = 1,
        .episode = 1
    };

    lib->videos[1] = (Video){
        .title = strdup("Series 1 - S01E02"),
        .path = strdup("/series1/S01E02.mkv"),
        .season = 1,
        .episode = 2
    };

    lib->videos[2] = (Video){
        .title = strdup("Movie 1"),
        .path = strdup("/movies/movie1.mp4"),
        .season = -1,
        .episode = -1
    };

    return lib;
}

// Construir URL para streaming
char* get_video_url(const char* server_url, const char* video_path, const char* mode) {
    // Modos disponibles:
    // - "direct": /video/{path}       (directo, sin transcodificar)
    // - "hls":    /hls/{path}/playlist.m3u8  (streaming HLS)
    // - "legacy": /legacy/{path}      (H.264 compatible)

    char* url = malloc(1024);

    if (strcmp(mode, "hls") == 0) {
        snprintf(url, 1024, "%s/hls/%s/playlist.m3u8", server_url, video_path);
    } else if (strcmp(mode, "legacy") == 0) {
        snprintf(url, 1024, "%s/legacy/%s", server_url, video_path);
    } else { // direct
        snprintf(url, 1024, "%s/video/%s", server_url, video_path);
    }

    return url;
}

// Integración con Sokol
void sokol_integration_example() {
    const char* server = "http://192.168.1.100:8080";

    // 1. Obtener biblioteca del servidor
    VideoLibrary* library = fetch_library(server);

    printf("\nBiblioteca de videos:\n");
    for (int i = 0; i < library->count; i++) {
        Video* v = &library->videos[i];
        printf("  [%d] %s\n", i, v->title);

        // 2. Generar URLs de streaming
        char* url_direct = get_video_url(server, v->path, "direct");
        char* url_hls = get_video_url(server, v->path, "hls");

        printf("      Direct: %s\n", url_direct);
        printf("      HLS:    %s\n", url_hls);

        free(url_direct);
        free(url_hls);
    }

    // 3. En Sokol, usarías estas URLs para:
    //    - Descargar chunks de video
    //    - Decodificar con FFmpeg
    //    - Renderizar frames con sokol_gfx
    //    - Sincronizar audio con sokol_audio

    printf("\n[!] Para implementar reproducción real:\n");
    printf("    1. Usar libcurl para HTTP streaming\n");
    printf("    2. Usar FFmpeg para decodificar video\n");
    printf("    3. Subir frames a texturas GPU (sokol_gfx)\n");
    printf("    4. Reproducir audio (sokol_audio)\n");

    // Cleanup
    for (int i = 0; i < library->count; i++) {
        free(library->videos[i].title);
        free(library->videos[i].path);
    }
    free(library->videos);
    free(library);
}

int main() {
    printf("=== Ejemplo de Cliente HTTP para Sokol Media Player ===\n\n");
    sokol_integration_example();
    return 0;
}

/*
 * ARQUITECTURA COMPLETA:
 *
 * ┌─────────────────────────────────────────┐
 * │   Backend C++ (media_server)            │
 * │   - Escanea biblioteca local            │
 * │   - Sirve API REST (/api/library)       │
 * │   - Streaming HTTP (/video, /hls)       │
 * │   - Transcodificación bajo demanda      │
 * └──────────────┬──────────────────────────┘
 *                │ HTTP
 *                │ JSON/Video Stream
 * ┌──────────────▼──────────────────────────┐
 * │   Cliente Sokol (sokol_media_player)    │
 * │   - UI nativa (sokol_app)               │
 * │   - HTTP client (libcurl)               │
 * │   - Video decode (FFmpeg)               │
 * │   - Renderizado (sokol_gfx + GPU)       │
 * │   - Audio (sokol_audio)                 │
 * └─────────────────────────────────────────┘
 *
 * PLATAFORMAS SOPORTADAS:
 * - Windows (D3D11)
 * - Linux (OpenGL 3.3)
 * - Android (OpenGL ES 3)
 *
 * El mismo código C compila para todas las plataformas!
 */
