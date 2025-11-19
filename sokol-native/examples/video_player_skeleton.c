/*
 * Video Player Skeleton - Estructura básica de un reproductor
 *
 * Este ejemplo muestra la estructura de un reproductor de video
 * cross-platform usando Sokol + FFmpeg (conceptual)
 */

#define SOKOL_IMPL
#if defined(_WIN32)
    #define SOKOL_D3D11
#elif defined(__ANDROID__)
    #define SOKOL_GLES3
#else
    #define SOKOL_GLCORE33
#endif

#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_time.h"
#include "sokol_audio.h"
#include "sokol_log.h"
#include "sokol_glue.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// Estado del reproductor de video
typedef struct {
    // Texturas para YUV (formato común de video)
    sg_image y_texture;   // Luma (brillo)
    sg_image u_texture;   // Chroma U (azul-amarillo)
    sg_image v_texture;   // Chroma V (rojo-verde)

    // Pipeline de renderizado
    sg_pipeline pip;
    sg_bindings bind;
    sg_pass_action pass_action;

    // Estado de reproducción
    bool playing;
    bool paused;
    double playback_time;
    uint64_t frame_timer;

    // Info del video
    int video_width;
    int video_height;
    double fps;

    // Buffer de audio
    float* audio_buffer;
    int audio_buffer_size;
    int audio_read_pos;

} video_player_t;

static video_player_t player;

// Vertex shader para renderizar video YUV
static const char* vs_yuv =
    "#version 330\n"
    "layout(location=0) in vec2 pos;\n"
    "layout(location=1) in vec2 uv;\n"
    "out vec2 tex_coord;\n"
    "void main() {\n"
    "  gl_Position = vec4(pos, 0.0, 1.0);\n"
    "  tex_coord = uv;\n"
    "}\n";

// Fragment shader - Conversión YUV a RGB
static const char* fs_yuv =
    "#version 330\n"
    "uniform sampler2D y_tex;\n"
    "uniform sampler2D u_tex;\n"
    "uniform sampler2D v_tex;\n"
    "in vec2 tex_coord;\n"
    "out vec4 frag_color;\n"
    "void main() {\n"
    "  float y = texture(y_tex, tex_coord).r;\n"
    "  float u = texture(u_tex, tex_coord).r - 0.5;\n"
    "  float v = texture(v_tex, tex_coord).r - 0.5;\n"
    "  float r = y + 1.402 * v;\n"
    "  float g = y - 0.344 * u - 0.714 * v;\n"
    "  float b = y + 1.772 * u;\n"
    "  frag_color = vec4(r, g, b, 1.0);\n"
    "}\n";

// Callback de audio - reproduce buffer de audio del video
static void audio_callback(float* buffer, int num_frames, int num_channels) {
    if (!player.playing || player.audio_buffer == NULL) {
        // Silencio
        for (int i = 0; i < num_frames * num_channels; i++) {
            buffer[i] = 0.0f;
        }
        return;
    }

    // Copiar desde buffer de audio decodificado
    for (int i = 0; i < num_frames * num_channels; i++) {
        if (player.audio_read_pos < player.audio_buffer_size) {
            buffer[i] = player.audio_buffer[player.audio_read_pos++];
        } else {
            buffer[i] = 0.0f;
        }
    }
}

// Decodificar siguiente frame (placeholder - usar FFmpeg real)
typedef struct {
    unsigned char* y_data;
    unsigned char* u_data;
    unsigned char* v_data;
    int width;
    int height;
} video_frame_t;

static bool decode_next_frame(video_frame_t* frame) {
    // En un reproductor real:
    // 1. Leer packet del video con av_read_frame()
    // 2. Decodificar con avcodec_send_packet() / avcodec_receive_frame()
    // 3. Convertir formato con sws_scale() si es necesario
    // 4. Extraer planes YUV del AVFrame

    // Por ahora, generar frame de prueba (gris)
    static bool initialized = false;
    if (!initialized) {
        frame->width = 1280;
        frame->height = 720;
        frame->y_data = malloc(frame->width * frame->height);
        frame->u_data = malloc((frame->width/2) * (frame->height/2));
        frame->v_data = malloc((frame->width/2) * (frame->height/2));

        memset(frame->y_data, 128, frame->width * frame->height);
        memset(frame->u_data, 128, (frame->width/2) * (frame->height/2));
        memset(frame->v_data, 128, (frame->width/2) * (frame->height/2));

        initialized = true;
    }

    return true;
}

// Inicializar reproductor
static void init(void) {
    printf("Inicializando video player...\n");

    // Setup Sokol GFX
    sg_setup(&(sg_desc){
        .context = sapp_sgcontext(),
        .logger.func = slog_func,
    });

    // Setup Sokol Audio
    saudio_setup(&(saudio_desc){
        .sample_rate = 44100,
        .num_channels = 2,
        .stream_cb = audio_callback,
        .logger.func = slog_func,
    });

    // Setup Sokol Time
    stm_setup();

    // Crear texturas para YUV (placeholder 1280x720)
    player.video_width = 1280;
    player.video_height = 720;

    player.y_texture = sg_make_image(&(sg_image_desc){
        .width = player.video_width,
        .height = player.video_height,
        .pixel_format = SG_PIXELFORMAT_R8,
        .usage = SG_USAGE_STREAM,
    });

    player.u_texture = sg_make_image(&(sg_image_desc){
        .width = player.video_width / 2,
        .height = player.video_height / 2,
        .pixel_format = SG_PIXELFORMAT_R8,
        .usage = SG_USAGE_STREAM,
    });

    player.v_texture = sg_make_image(&(sg_image_desc){
        .width = player.video_width / 2,
        .height = player.video_height / 2,
        .pixel_format = SG_PIXELFORMAT_R8,
        .usage = SG_USAGE_STREAM,
    });

    // Crear quad para renderizar video
    float vertices[] = {
        // pos        // uv
        -1.0f,  1.0f, 0.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 0.0f,
        -1.0f, -1.0f, 0.0f, 1.0f,
         1.0f, -1.0f, 1.0f, 1.0f,
    };

    player.bind.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
        .data = SG_RANGE(vertices),
    });

    // Crear shader YUV
    sg_shader shd = sg_make_shader(&(sg_shader_desc){
        .vs.source = vs_yuv,
        .fs.source = fs_yuv,
        .fs.images = {
            [0] = { .name = "y_tex", .image_type = SG_IMAGETYPE_2D },
            [1] = { .name = "u_tex", .image_type = SG_IMAGETYPE_2D },
            [2] = { .name = "v_tex", .image_type = SG_IMAGETYPE_2D },
        },
    });

    // Pipeline
    player.pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .layout = {
            .attrs = {
                [0].format = SG_VERTEXFORMAT_FLOAT2,
                [1].format = SG_VERTEXFORMAT_FLOAT2,
            }
        },
        .primitive_type = SG_PRIMITIVETYPE_TRIANGLE_STRIP,
    });

    player.bind.fs_images[0] = player.y_texture;
    player.bind.fs_images[1] = player.u_texture;
    player.bind.fs_images[2] = player.v_texture;

    player.pass_action = (sg_pass_action){
        .colors[0] = { .load_action = SG_LOADACTION_CLEAR, .clear_value = {0.0f, 0.0f, 0.0f, 1.0f} }
    };

    player.playing = true;
    player.paused = false;
    player.frame_timer = stm_now();

    printf("Video player inicializado!\n");
}

// Frame callback
static void frame(void) {
    // Calcular tiempo de frame
    uint64_t now = stm_now();
    double dt = stm_sec(stm_diff(now, player.frame_timer));

    if (player.playing && !player.paused) {
        player.playback_time += dt;

        // Decodificar y actualizar frame (placeholder)
        video_frame_t video_frame;
        if (decode_next_frame(&video_frame)) {
            // Actualizar texturas con datos del frame
            sg_update_image(player.y_texture, &(sg_image_data){
                .subimage[0][0] = {
                    .ptr = video_frame.y_data,
                    .size = video_frame.width * video_frame.height
                }
            });

            sg_update_image(player.u_texture, &(sg_image_data){
                .subimage[0][0] = {
                    .ptr = video_frame.u_data,
                    .size = (video_frame.width/2) * (video_frame.height/2)
                }
            });

            sg_update_image(player.v_texture, &(sg_image_data){
                .subimage[0][0] = {
                    .ptr = video_frame.v_data,
                    .size = (video_frame.width/2) * (video_frame.height/2)
                }
            });
        }
    }

    player.frame_timer = now;

    // Renderizar
    sg_begin_default_pass(&player.pass_action, sapp_width(), sapp_height());
    sg_apply_pipeline(player.pip);
    sg_apply_bindings(&player.bind);
    sg_draw(0, 4, 1);
    sg_end_pass();
    sg_commit();
}

// Input handler
static void input_event(const sapp_event* e) {
    switch (e->type) {
        case SAPP_EVENTTYPE_KEY_DOWN:
            if (e->key_code == SAPP_KEYCODE_SPACE) {
                player.paused = !player.paused;
                printf("Reproducción %s\n", player.paused ? "pausada" : "reanudada");
            }
            break;

        case SAPP_EVENTTYPE_TOUCHES_BEGAN:
        case SAPP_EVENTTYPE_MOUSE_DOWN:
            player.paused = !player.paused;
            break;

        default:
            break;
    }
}

// Cleanup
static void cleanup(void) {
    saudio_shutdown();
    sg_shutdown();
}

// Entry point
sapp_desc sokol_main(int argc, char* argv[]) {
    (void)argc; (void)argv;

    return (sapp_desc){
        .init_cb = init,
        .frame_cb = frame,
        .cleanup_cb = cleanup,
        .event_cb = input_event,
        .width = 1280,
        .height = 720,
        .window_title = "Sokol Video Player",
        .logger.func = slog_func,
    };
}

/*
 * PARA IMPLEMENTAR REPRODUCCIÓN REAL:
 *
 * 1. Integrar FFmpeg:
 *    - avformat_open_input() - Abrir archivo/stream
 *    - avcodec_find_decoder() - Encontrar decoder
 *    - avcodec_open2() - Inicializar decoder
 *    - av_read_frame() - Leer packets
 *    - avcodec_send_packet() / avcodec_receive_frame() - Decodificar
 *
 * 2. Sincronización A/V:
 *    - Usar PTS (Presentation Timestamp) de frames
 *    - Calcular timing con sokol_time.h
 *    - Ajustar reproducción de audio
 *
 * 3. Controles:
 *    - Play/Pause
 *    - Seek (saltar a tiempo específico)
 *    - Volumen
 *    - Pantalla completa
 *
 * 4. Subtítulos:
 *    - Decodificar stream de subtítulos
 *    - Renderizar texto con sokol_gfx
 *
 * 5. Performance:
 *    - Decodificar en thread separado
 *    - Ring buffer para frames
 *    - Hardware decode si disponible (VAAPI, VDPAU, VideoToolbox)
 */
