/*
 * Simple Media Player - Sokol Native Client
 * DEBUG MODE: Extensive terminal logging for headless debugging
 *
 * This version logs EVERYTHING to terminal so Claude can debug without GUI access
 */

#define SOKOL_IMPL
#if defined(_WIN32)
    #define SOKOL_D3D11
#elif defined(__ANDROID__)
    #define SOKOL_GLES3
#elif defined(__linux__)
    #define SOKOL_GLCORE  // Note: GLCORE not GLCORE33
#endif

#include "../external/sokol/sokol_log.h"
#include "../external/sokol/sokol_app.h"
#include "../external/sokol/sokol_gfx.h"
#include "../external/sokol/sokol_glue.h"
#include "../external/sokol/sokol_time.h"
#include "../external/sokol/sokol_audio.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

// ============================================
// DEBUG CONFIGURATION
// ============================================
#define DEBUG_VERBOSE 1          // Enable verbose logging
#define DEBUG_FRAME_LOG 60       // Log every N frames
#define DEBUG_VALIDATION 1       // Validate state on each operation
#define TEST_DURATION_FRAMES 300 // Auto-quit after N frames for testing

// ============================================
// APPLICATION STATE
// ============================================
typedef struct {
    sg_pass_action pass_action;
    sg_pipeline pip;
    sg_bindings bind;
    float angle;
    bool initialized;
    uint64_t last_time;
    int frame_count;
    int error_count;
    bool validation_passed;
} app_state_t;

static app_state_t state = {0};

// ============================================
// LOGGING HELPERS
// ============================================
#undef LOG_INFO
#undef LOG_DEBUG
#define LOG_INFO(fmt, ...) printf("[INFO] " fmt "\n", ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) do { printf("[ERROR] " fmt "\n", ##__VA_ARGS__); state.error_count++; } while(0)
#define LOG_SUCCESS(fmt, ...) printf("[✓] " fmt "\n", ##__VA_ARGS__)
#define LOG_WARN(fmt, ...) printf("[WARN] " fmt "\n", ##__VA_ARGS__)

// Validate Sokol state
static bool validate_sokol_gfx_state(const char* checkpoint) {
    sg_backend backend = sg_query_backend();
    bool valid = sg_isvalid();

    LOG_DEBUG("Validation at '%s':", checkpoint);
    LOG_DEBUG("  - Backend: %s",
        backend == SG_BACKEND_GLCORE ? "OpenGL Core" :
        backend == SG_BACKEND_GLES3 ? "OpenGL ES 3" :
        backend == SG_BACKEND_D3D11 ? "D3D11" :
        backend == SG_BACKEND_METAL_IOS ? "Metal iOS" :
        backend == SG_BACKEND_METAL_MACOS ? "Metal macOS" :
        "Unknown");
    LOG_DEBUG("  - sg_isvalid(): %s", valid ? "TRUE" : "FALSE");

    if (!valid) {
        LOG_ERROR("Sokol GFX validation FAILED at '%s'", checkpoint);
        return false;
    }

    LOG_SUCCESS("Sokol GFX validation passed at '%s'", checkpoint);
    return true;
}

// ============================================
// AUDIO CALLBACK
// ============================================
static void audio_callback(float* buffer, int num_frames, int num_channels) {
    static int callback_count = 0;

    // Generate silence (can enable test tone for audio verification)
    for (int i = 0; i < num_frames * num_channels; i++) {
        buffer[i] = 0.0f;
    }

    // Log every 1000 callbacks (~22 seconds at 44.1kHz with 1024 buffer)
    if (++callback_count % 1000 == 0) {
        LOG_DEBUG("Audio callback #%d (frames=%d, channels=%d)",
                  callback_count, num_frames, num_channels);
    }
}

// ============================================
// SHADERS
// ============================================
static const char* vs_source_gl33 =
    "#version 330\n"
    "layout(location=0) in vec4 position;\n"
    "layout(location=1) in vec4 color0;\n"
    "out vec4 color;\n"
    "uniform mat4 mvp;\n"
    "void main() {\n"
    "  gl_Position = mvp * position;\n"
    "  color = color0;\n"
    "}\n";

static const char* fs_source_gl33 =
    "#version 330\n"
    "in vec4 color;\n"
    "out vec4 frag_color;\n"
    "void main() {\n"
    "  frag_color = color;\n"
    "}\n";

// ============================================
// INITIALIZATION
// ============================================
static void init(void) {
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  SOKOL MEDIA PLAYER - DEBUG MODE\n");
    printf("  Extensive logging for headless debugging\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("\n");

    LOG_INFO("Platform: %s",
#if defined(_WIN32)
        "Windows"
#elif defined(__ANDROID__)
        "Android"
#elif defined(__linux__)
        "Linux"
#elif defined(__APPLE__)
        "macOS/iOS"
#else
        "Unknown"
#endif
    );

    LOG_INFO("Sokol backend: %s",
#if defined(SOKOL_D3D11)
        "Direct3D 11"
#elif defined(SOKOL_GLES3)
        "OpenGL ES 3"
#elif defined(SOKOL_GLCORE33)
        "OpenGL 3.3 Core"
#elif defined(SOKOL_METAL)
        "Metal"
#else
        "Unknown"
#endif
    );

    // ========================================
    // STEP 1: Initialize Sokol GFX
    // ========================================
    LOG_INFO("Step 1/5: Initializing Sokol GFX...");
    sg_setup(&(sg_desc){
        .environment = sglue_environment(),
        .logger.func = slog_func,
    });

    if (!validate_sokol_gfx_state("After sg_setup()")) {
        LOG_ERROR("CRITICAL: Sokol GFX setup failed!");
        state.validation_passed = false;
        return;
    }

    LOG_SUCCESS("Sokol GFX initialized");

    // ========================================
    // STEP 2: Initialize Sokol Audio
    // ========================================
    LOG_INFO("Step 2/5: Initializing Sokol Audio...");
    saudio_setup(&(saudio_desc){
        .sample_rate = 44100,
        .num_channels = 2,
        .buffer_frames = 1024,
        .stream_cb = audio_callback,
        .logger.func = slog_func,
    });

    bool audio_valid = saudio_isvalid();
    LOG_DEBUG("  - Sample rate: %d Hz", saudio_sample_rate());
    LOG_DEBUG("  - Channels: %d", saudio_channels());
    LOG_DEBUG("  - Buffer frames: %d", saudio_buffer_frames());
    LOG_DEBUG("  - saudio_isvalid(): %s", audio_valid ? "TRUE" : "FALSE");

    if (!audio_valid) {
        LOG_WARN("Audio initialization failed (non-critical, continuing)");
    } else {
        LOG_SUCCESS("Sokol Audio initialized");
    }

    // ========================================
    // STEP 3: Initialize Sokol Time
    // ========================================
    LOG_INFO("Step 3/5: Initializing Sokol Time...");
    stm_setup();
    state.last_time = stm_now();
    LOG_DEBUG("  - Initial timestamp: %llu", (unsigned long long)state.last_time);
    LOG_SUCCESS("Sokol Time initialized");

    // ========================================
    // STEP 4: Create Graphics Resources
    // ========================================
    LOG_INFO("Step 4/5: Creating graphics resources...");

    // Create vertex buffer
    LOG_DEBUG("Creating vertex buffer...");
    float vertices[] = {
        // positions         // colors
         0.0f,  0.5f, 0.5f,  1.0f, 0.0f, 0.0f, 1.0f,  // top (red)
         0.5f, -0.5f, 0.5f,  0.0f, 1.0f, 0.0f, 1.0f,  // right (green)
        -0.5f, -0.5f, 0.5f,  0.0f, 0.0f, 1.0f, 1.0f,  // left (blue)
    };

    state.bind.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
        .data = SG_RANGE(vertices),
        .label = "triangle-vertices"
    });

    sg_resource_state vb_state = sg_query_buffer_state(state.bind.vertex_buffers[0]);
    LOG_DEBUG("  - Vertex buffer state: %s",
        vb_state == SG_RESOURCESTATE_VALID ? "VALID" :
        vb_state == SG_RESOURCESTATE_FAILED ? "FAILED" :
        vb_state == SG_RESOURCESTATE_INVALID ? "INVALID" : "UNKNOWN");

    if (vb_state != SG_RESOURCESTATE_VALID) {
        LOG_ERROR("Vertex buffer creation FAILED!");
        state.validation_passed = false;
        return;
    }
    LOG_SUCCESS("Vertex buffer created (84 bytes)");

    // Create shader
    LOG_DEBUG("Creating shader...");
    sg_shader shd = sg_make_shader(&(sg_shader_desc){
        .vertex_func.source = vs_source_gl33,
        .fragment_func.source = fs_source_gl33,
        .uniform_blocks[0] = {
            .stage = SG_SHADERSTAGE_VERTEX,
            .size = 64,
            .layout = SG_UNIFORMLAYOUT_NATIVE,
            .glsl_uniforms = {
                [0] = { .type = SG_UNIFORMTYPE_MAT4, .array_count = 1, .glsl_name = "mvp" }
            }
        },
        .label = "triangle-shader"
    });

    sg_resource_state shd_state = sg_query_shader_state(shd);
    LOG_DEBUG("  - Shader state: %s",
        shd_state == SG_RESOURCESTATE_VALID ? "VALID" :
        shd_state == SG_RESOURCESTATE_FAILED ? "FAILED" :
        shd_state == SG_RESOURCESTATE_INVALID ? "INVALID" : "UNKNOWN");

    if (shd_state != SG_RESOURCESTATE_VALID) {
        LOG_ERROR("Shader creation FAILED!");
        state.validation_passed = false;
        return;
    }
    LOG_SUCCESS("Shader compiled and linked");

    // Create pipeline
    LOG_DEBUG("Creating pipeline...");
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

    sg_resource_state pip_state = sg_query_pipeline_state(state.pip);
    LOG_DEBUG("  - Pipeline state: %s",
        pip_state == SG_RESOURCESTATE_VALID ? "VALID" :
        pip_state == SG_RESOURCESTATE_FAILED ? "FAILED" :
        pip_state == SG_RESOURCESTATE_INVALID ? "INVALID" : "UNKNOWN");

    if (pip_state != SG_RESOURCESTATE_VALID) {
        LOG_ERROR("Pipeline creation FAILED!");
        state.validation_passed = false;
        return;
    }
    LOG_SUCCESS("Pipeline created");

    // Configure pass action
    state.pass_action = (sg_pass_action) {
        .colors[0] = {
            .load_action = SG_LOADACTION_CLEAR,
            .clear_value = {0.1f, 0.1f, 0.1f, 1.0f}
        }
    };
    LOG_DEBUG("Pass action configured (clear color: dark gray)");

    // ========================================
    // STEP 5: Final Validation
    // ========================================
    LOG_INFO("Step 5/5: Final validation...");

    if (!validate_sokol_gfx_state("Final validation")) {
        LOG_ERROR("Final validation FAILED!");
        state.validation_passed = false;
        return;
    }

    state.initialized = true;
    state.validation_passed = true;
    state.frame_count = 0;
    state.angle = 0.0f;

    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  ✓ INITIALIZATION COMPLETE\n");
    printf("  Errors: %d\n", state.error_count);
    printf("  Validation: %s\n", state.validation_passed ? "PASSED" : "FAILED");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("\n");

    LOG_INFO("Controls:");
    LOG_INFO("  - SPACE: Reset rotation");
    LOG_INFO("  - ESC: Quit");
    LOG_INFO("  - Click/Touch: Log interaction");
    printf("\n");
    LOG_INFO("Will auto-quit after %d frames for testing", TEST_DURATION_FRAMES);
    printf("\n");
}

// ============================================
// MATRIX HELPERS
// ============================================
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

// ============================================
// FRAME CALLBACK
// ============================================
static void frame(void) {
    if (!state.initialized) {
        LOG_ERROR("Frame called but not initialized!");
        return;
    }

    state.frame_count++;

    // Calculate delta time
    uint64_t now = stm_now();
    double dt = stm_sec(stm_diff(now, state.last_time));
    state.last_time = now;

    // Update rotation
    state.angle += dt * 1.0f;  // 1 radian per second

    // Periodic logging
    if (state.frame_count % DEBUG_FRAME_LOG == 0) {
        LOG_DEBUG("Frame %d | dt=%.3fms | angle=%.2f rad | fps=~%.1f",
                  state.frame_count, dt * 1000.0, state.angle, 1.0 / dt);
    }

    // Create rotation matrix
    float mvp[16];
    mat4_rotate_z(mvp, state.angle);

    // Begin rendering
    int width = sapp_width();
    int height = sapp_height();

    if (state.frame_count == 1) {
        LOG_INFO("First frame render:");
        LOG_INFO("  - Viewport: %dx%d", width, height);
    }

    sg_begin_pass(&(sg_pass){
        .action = state.pass_action,
        .swapchain = sglue_swapchain()
    });
    sg_apply_pipeline(state.pip);
    sg_apply_bindings(&state.bind);
    sg_apply_uniforms(0, &SG_RANGE(mvp));
    sg_draw(0, 3, 1);
    sg_end_pass();
    sg_commit();

    // Validate every 60 frames
    if (DEBUG_VALIDATION && state.frame_count % 60 == 0) {
        if (!validate_sokol_gfx_state("Frame validation")) {
            LOG_ERROR("Frame %d validation failed!", state.frame_count);
        }
    }

    // Auto-quit for testing
    if (state.frame_count >= TEST_DURATION_FRAMES) {
        LOG_INFO("Reached %d frames, auto-quitting for test", TEST_DURATION_FRAMES);
        LOG_INFO("Test completed successfully!");
        sapp_request_quit();
    }
}

// ============================================
// INPUT EVENT HANDLER
// ============================================
static void input_event(const sapp_event* e) {
    switch (e->type) {
        case SAPP_EVENTTYPE_KEY_DOWN:
            LOG_INFO("Key pressed: code=%d, repeat=%s",
                     e->key_code, e->key_repeat ? "yes" : "no");

            if (e->key_code == SAPP_KEYCODE_ESCAPE) {
                LOG_INFO("ESC pressed - requesting quit");
                sapp_request_quit();
            } else if (e->key_code == SAPP_KEYCODE_SPACE) {
                LOG_INFO("SPACE pressed - resetting rotation");
                state.angle = 0.0f;
            }
            break;

        case SAPP_EVENTTYPE_KEY_UP:
            LOG_DEBUG("Key released: code=%d", e->key_code);
            break;

        case SAPP_EVENTTYPE_MOUSE_DOWN:
            LOG_INFO("Mouse button %d pressed at (%.1f, %.1f)",
                     e->mouse_button, e->mouse_x, e->mouse_y);
            break;

        case SAPP_EVENTTYPE_MOUSE_UP:
            LOG_DEBUG("Mouse button %d released at (%.1f, %.1f)",
                      e->mouse_button, e->mouse_x, e->mouse_y);
            break;

        case SAPP_EVENTTYPE_MOUSE_MOVE:
            // Too verbose, only log on first occurrence
            if (state.frame_count == 1) {
                LOG_DEBUG("Mouse move detected (will not log further)");
            }
            break;

        case SAPP_EVENTTYPE_TOUCHES_BEGAN:
            LOG_INFO("Touch began: id=%lu, pos=(%.1f, %.1f)",
                     (unsigned long)e->touches[0].identifier,
                     e->touches[0].pos_x, e->touches[0].pos_y);
            break;

        case SAPP_EVENTTYPE_TOUCHES_ENDED:
            LOG_INFO("Touch ended: id=%lu",
                     (unsigned long)e->touches[0].identifier);
            break;

        case SAPP_EVENTTYPE_RESIZED:
            LOG_INFO("Window resized to %dx%d", sapp_width(), sapp_height());
            break;

        case SAPP_EVENTTYPE_SUSPENDED:
            LOG_INFO("Application suspended");
            break;

        case SAPP_EVENTTYPE_RESUMED:
            LOG_INFO("Application resumed");
            break;

        default:
            break;
    }
}

// ============================================
// CLEANUP
// ============================================
static void cleanup(void) {
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  CLEANUP & STATISTICS\n");
    printf("═══════════════════════════════════════════════════════════\n");

    LOG_INFO("Total frames rendered: %d", state.frame_count);
    LOG_INFO("Total errors: %d", state.error_count);
    LOG_INFO("Final validation: %s", state.validation_passed ? "PASSED" : "FAILED");

    LOG_INFO("Shutting down audio...");
    saudio_shutdown();
    LOG_SUCCESS("Audio shutdown complete");

    LOG_INFO("Shutting down graphics...");
    sg_shutdown();
    LOG_SUCCESS("Graphics shutdown complete");

    printf("═══════════════════════════════════════════════════════════\n");

    if (state.error_count > 0) {
        printf("  ⚠ COMPLETED WITH %d ERRORS\n", state.error_count);
    } else {
        printf("  ✓ COMPLETED SUCCESSFULLY\n");
    }

    printf("═══════════════════════════════════════════════════════════\n");
    printf("\n");
}

// ============================================
// MAIN ENTRY POINT
// ============================================
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
        .window_title = "Sokol Media Player - DEBUG MODE",
        .ios_keyboard_resizes_canvas = false,
        .icon.sokol_default = true,
        .logger.func = slog_func,
    };
}
