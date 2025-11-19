/*
 * Simple Media Player - Sokol Native Client
 * Cross-platform media player using Sokol headers
 * Supports: Windows, Linux, Android
 */

#define SOKOL_IMPL
#if defined(_WIN32)
    #define SOKOL_D3D11
#elif defined(__ANDROID__)
    #define SOKOL_GLES3
#elif defined(__linux__)
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
#include <string.h>

// Application state
typedef struct {
    sg_pass_action pass_action;
    sg_pipeline pip;
    sg_bindings bind;
    float angle;
    bool initialized;
    uint64_t last_time;
} app_state_t;

static app_state_t state;

// Audio callback - generates simple test tone
static void audio_callback(float* buffer, int num_frames, int num_channels) {
    static float phase = 0.0f;
    const float freq = 440.0f; // A4 note
    const float sample_rate = (float)saudio_sample_rate();
    const float phase_inc = (2.0f * 3.14159265f * freq) / sample_rate;

    for (int i = 0; i < num_frames; i++) {
        float sample = 0.0f; // Silence for now, can enable tone for testing
        // float sample = sinf(phase) * 0.1f; // Uncomment for test tone

        for (int ch = 0; ch < num_channels; ch++) {
            buffer[i * num_channels + ch] = sample;
        }

        phase += phase_inc;
        if (phase > 2.0f * 3.14159265f) {
            phase -= 2.0f * 3.14159265f;
        }
    }
}

// Vertex shader
static const char* vs_source =
    "#version 330\n"
    "layout(location=0) in vec4 position;\n"
    "layout(location=1) in vec4 color0;\n"
    "out vec4 color;\n"
    "uniform mat4 mvp;\n"
    "void main() {\n"
    "  gl_Position = mvp * position;\n"
    "  color = color0;\n"
    "}\n";

// Fragment shader
static const char* fs_source =
    "#version 330\n"
    "in vec4 color;\n"
    "out vec4 frag_color;\n"
    "void main() {\n"
    "  frag_color = color;\n"
    "}\n";

// For GLES3/Android
static const char* vs_source_gles =
    "#version 300 es\n"
    "layout(location=0) in vec4 position;\n"
    "layout(location=1) in vec4 color0;\n"
    "out vec4 color;\n"
    "uniform mat4 mvp;\n"
    "void main() {\n"
    "  gl_Position = mvp * position;\n"
    "  color = color0;\n"
    "}\n";

static const char* fs_source_gles =
    "#version 300 es\n"
    "precision mediump float;\n"
    "in vec4 color;\n"
    "out vec4 frag_color;\n"
    "void main() {\n"
    "  frag_color = color;\n"
    "}\n";

// Initialize application
static void init(void) {
    printf("Initializing Sokol Media Player...\n");

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

    printf("Graphics backend: %s\n", sg_query_backend() == SG_BACKEND_GLES3 ? "OpenGL ES 3" :
                                      sg_query_backend() == SG_BACKEND_GLCORE33 ? "OpenGL 3.3" :
                                      sg_query_backend() == SG_BACKEND_D3D11 ? "D3D11" : "Unknown");
    printf("Audio: %d Hz, %d channels\n", saudio_sample_rate(), saudio_channels());

    // Create a simple triangle for testing
    float vertices[] = {
        // positions            // colors
         0.0f,  0.5f, 0.5f,     1.0f, 0.0f, 0.0f, 1.0f,
         0.5f, -0.5f, 0.5f,     0.0f, 1.0f, 0.0f, 1.0f,
        -0.5f, -0.5f, 0.5f,     0.0f, 0.0f, 1.0f, 1.0f,
    };

    state.bind.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
        .data = SG_RANGE(vertices),
        .label = "triangle-vertices"
    });

    // Create shader
    #if defined(__ANDROID__)
        const char* vs = vs_source_gles;
        const char* fs = fs_source_gles;
    #else
        const char* vs = vs_source;
        const char* fs = fs_source;
    #endif

    sg_shader shd = sg_make_shader(&(sg_shader_desc){
        .vs.source = vs,
        .fs.source = fs,
        .vs.uniform_blocks[0] = {
            .size = 64, // mat4
            .uniforms = {
                [0] = { .name = "mvp", .type = SG_UNIFORMTYPE_MAT4 }
            }
        },
        .label = "triangle-shader"
    });

    // Create pipeline
    state.pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .layout = {
            .attrs = {
                [0].format = SG_VERTEXFORMAT_FLOAT3,
                [1].format = SG_VERTEXFORMAT_FLOAT4
            }
        },
        .label = "triangle-pipeline"
    });

    // Pass action (clear color)
    state.pass_action = (sg_pass_action) {
        .colors[0] = { .load_action=SG_LOADACTION_CLEAR, .clear_value={0.1f, 0.1f, 0.1f, 1.0f} }
    };

    state.initialized = true;
    state.last_time = stm_now();

    printf("Initialization complete!\n");
}

// Simple matrix multiplication helper
static void mat4_identity(float* m) {
    memset(m, 0, 16 * sizeof(float));
    m[0] = m[5] = m[10] = m[15] = 1.0f;
}

static void mat4_rotate_z(float* m, float angle) {
    mat4_identity(m);
    float c = cosf(angle);
    float s = sinf(angle);
    m[0] = c;  m[1] = s;
    m[4] = -s; m[5] = c;
}

// Frame callback
static void frame(void) {
    // Calculate delta time
    uint64_t now = stm_now();
    float dt = (float)stm_sec(stm_diff(now, state.last_time));
    state.last_time = now;

    // Rotate triangle
    state.angle += dt * 1.0f;

    // Create rotation matrix
    float mvp[16];
    mat4_rotate_z(mvp, state.angle);

    // Begin rendering
    sg_begin_default_pass(&state.pass_action, sapp_width(), sapp_height());
    sg_apply_pipeline(state.pip);
    sg_apply_bindings(&state.bind);
    sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &SG_RANGE(mvp));
    sg_draw(0, 3, 1);
    sg_end_pass();
    sg_commit();
}

// Input event handler
static void input_event(const sapp_event* e) {
    switch (e->type) {
        case SAPP_EVENTTYPE_KEY_DOWN:
            printf("Key pressed: %d\n", e->key_code);
            if (e->key_code == SAPP_KEYCODE_ESCAPE) {
                sapp_request_quit();
            }
            break;

        case SAPP_EVENTTYPE_TOUCHES_BEGAN:
        case SAPP_EVENTTYPE_MOUSE_DOWN:
            printf("Touch/Click at: %.1f, %.1f\n",
                   e->type == SAPP_EVENTTYPE_TOUCHES_BEGAN ? e->touches[0].pos_x : e->mouse_x,
                   e->type == SAPP_EVENTTYPE_TOUCHES_BEGAN ? e->touches[0].pos_y : e->mouse_y);
            break;

        default:
            break;
    }
}

// Cleanup
static void cleanup(void) {
    printf("Shutting down...\n");
    saudio_shutdown();
    sg_shutdown();
}

// Main entry point
sapp_desc sokol_main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    return (sapp_desc){
        .init_cb = init,
        .frame_cb = frame,
        .cleanup_cb = cleanup,
        .event_cb = input_event,
        .width = 800,
        .height = 600,
        .window_title = "Sokol Media Player",
        .ios_keyboard_resizes_canvas = false,
        .icon.sokol_default = true,
        .logger.func = slog_func,

        // Android-specific
        #if defined(__ANDROID__)
        .fullscreen = false,
        .enable_clipboard = false,
        #endif
    };
}
