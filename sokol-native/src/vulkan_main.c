/*
 * Simple Media Player - Native Vulkan Implementation
 * Cross-platform: Linux, Windows, Android
 *
 * This implementation uses:
 * - sokol_app.h: Window management and input (platform abstraction)
 * - Vulkan API: Full control over GPU rendering
 *
 * Maximum versatility for future hardware extensions
 */

// Define a dummy backend for sokol_app (we only use it for windowing, not rendering)
#if defined(_WIN32)
    #define SOKOL_D3D11
#elif defined(__ANDROID__)
    #define SOKOL_GLES3
#else
    #define SOKOL_GLCORE
#endif

#define SOKOL_IMPL
#include "../external/sokol/sokol_app.h"
#include "../external/sokol/sokol_time.h"
#include "../external/sokol/sokol_audio.h"
#include "../external/sokol/sokol_log.h"

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

// Platform-specific Vulkan extensions
#if defined(__linux__) && !defined(__ANDROID__)
#include <vulkan/vulkan_xlib.h>
#elif defined(_WIN32)
#include <vulkan/vulkan_win32.h>
#elif defined(__ANDROID__)
#include <vulkan/vulkan_android.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

// ============================================
// DEBUG CONFIGURATION
// ============================================
#define DEBUG_VERBOSE 1
#define DEBUG_FRAME_LOG 60
#define TEST_DURATION_FRAMES 300

// ============================================
// LOGGING MACROS
// ============================================
#undef LOG_INFO
#undef LOG_DEBUG
#define LOG_INFO(fmt, ...) printf("[INFO] " fmt "\n", ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) printf("[ERROR] " fmt "\n", ##__VA_ARGS__)
#define LOG_SUCCESS(fmt, ...) printf("[✓] " fmt "\n", ##__VA_ARGS__)
#define LOG_WARN(fmt, ...) printf("[WARN] " fmt "\n", ##__VA_ARGS__)

#define VK_CHECK(call) do { \
    VkResult result = call; \
    if (result != VK_SUCCESS) { \
        LOG_ERROR(#call " failed with result %d", result); \
        return false; \
    } \
} while(0)

// ============================================
// VULKAN STATE
// ============================================
typedef struct {
    // Core Vulkan objects
    VkInstance instance;
    VkPhysicalDevice physical_device;
    VkDevice device;
    VkQueue graphics_queue;
    VkQueue present_queue;
    uint32_t graphics_family_index;
    uint32_t present_family_index;

    // Surface and swapchain
    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain;
    VkFormat swapchain_format;
    VkExtent2D swapchain_extent;
    VkImage* swapchain_images;
    VkImageView* swapchain_image_views;
    uint32_t swapchain_image_count;

    // Rendering
    VkRenderPass render_pass;
    VkPipelineLayout pipeline_layout;
    VkPipeline graphics_pipeline;
    VkFramebuffer* framebuffers;

    // Command buffers
    VkCommandPool command_pool;
    VkCommandBuffer* command_buffers;

    // Synchronization
    VkSemaphore image_available_semaphore;
    VkSemaphore render_finished_semaphore;
    VkFence in_flight_fence;

    // Validation layer
    VkDebugUtilsMessengerEXT debug_messenger;

    // App state
    bool initialized;
    int frame_count;
    int error_count;
    uint64_t last_time;
    float angle;

} vulkan_state_t;

static vulkan_state_t vk_state = {0};

// ============================================
// VULKAN FUNCTION POINTERS
// ============================================
PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
PFN_vkCreateInstance vkCreateInstance;
PFN_vkEnumerateInstanceExtensionProperties vkEnumerateInstanceExtensionProperties;
PFN_vkEnumerateInstanceLayerProperties vkEnumerateInstanceLayerProperties;

// Instance-level functions
PFN_vkDestroyInstance vkDestroyInstance;
PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices;
PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties;
PFN_vkGetPhysicalDeviceFeatures vkGetPhysicalDeviceFeatures;
PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties;
PFN_vkGetPhysicalDeviceSurfaceSupportKHR vkGetPhysicalDeviceSurfaceSupportKHR;
PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR;
PFN_vkGetPhysicalDeviceSurfacePresentModesKHR vkGetPhysicalDeviceSurfacePresentModesKHR;
PFN_vkCreateDevice vkCreateDevice;
PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr;
PFN_vkDestroySurfaceKHR vkDestroySurfaceKHR;
PFN_vkEnumerateDeviceExtensionProperties vkEnumerateDeviceExtensionProperties;

// Platform-specific surface creation
#if defined(_WIN32)
PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR;
#elif defined(__ANDROID__)
PFN_vkCreateAndroidSurfaceKHR vkCreateAndroidSurfaceKHR;
#elif defined(__linux__)
PFN_vkCreateXlibSurfaceKHR vkCreateXlibSurfaceKHR;
#endif

// Device-level functions
PFN_vkGetDeviceQueue vkGetDeviceQueue;
PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR;
PFN_vkDestroySwapchainKHR vkDestroySwapchainKHR;
PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR;
PFN_vkCreateImageView vkCreateImageView;
PFN_vkDestroyImageView vkDestroyImageView;
PFN_vkCreateRenderPass vkCreateRenderPass;
PFN_vkDestroyRenderPass vkDestroyRenderPass;
PFN_vkCreateShaderModule vkCreateShaderModule;
PFN_vkDestroyShaderModule vkDestroyShaderModule;
PFN_vkCreatePipelineLayout vkCreatePipelineLayout;
PFN_vkDestroyPipelineLayout vkDestroyPipelineLayout;
PFN_vkCreateGraphicsPipelines vkCreateGraphicsPipelines;
PFN_vkDestroyPipeline vkDestroyPipeline;
PFN_vkCreateFramebuffer vkCreateFramebuffer;
PFN_vkDestroyFramebuffer vkDestroyFramebuffer;
PFN_vkCreateCommandPool vkCreateCommandPool;
PFN_vkDestroyCommandPool vkDestroyCommandPool;
PFN_vkAllocateCommandBuffers vkAllocateCommandBuffers;
PFN_vkFreeCommandBuffers vkFreeCommandBuffers;
PFN_vkBeginCommandBuffer vkBeginCommandBuffer;
PFN_vkEndCommandBuffer vkEndCommandBuffer;
PFN_vkResetCommandBuffer vkResetCommandBuffer;
PFN_vkCmdBeginRenderPass vkCmdBeginRenderPass;
PFN_vkCmdEndRenderPass vkCmdEndRenderPass;
PFN_vkCmdBindPipeline vkCmdBindPipeline;
PFN_vkCmdDraw vkCmdDraw;
PFN_vkCreateSemaphore vkCreateSemaphore;
PFN_vkDestroySemaphore vkDestroySemaphore;
PFN_vkCreateFence vkCreateFence;
PFN_vkDestroyFence vkDestroyFence;
PFN_vkWaitForFences vkWaitForFences;
PFN_vkResetFences vkResetFences;
PFN_vkAcquireNextImageKHR vkAcquireNextImageKHR;
PFN_vkQueueSubmit vkQueueSubmit;
PFN_vkQueuePresentKHR vkQueuePresentKHR;
PFN_vkDeviceWaitIdle vkDeviceWaitIdle;
PFN_vkDestroyDevice vkDestroyDevice;

// Debug messenger
PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT;
PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT;

// ============================================
// VULKAN LOADER
// ============================================
#if defined(_WIN32)
#include <windows.h>
static HMODULE vulkan_library = NULL;

bool load_vulkan_loader() {
    vulkan_library = LoadLibraryA("vulkan-1.dll");
    if (!vulkan_library) {
        LOG_ERROR("Failed to load vulkan-1.dll");
        return false;
    }
    vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)GetProcAddress(vulkan_library, "vkGetInstanceProcAddr");
    if (!vkGetInstanceProcAddr) {
        LOG_ERROR("Failed to get vkGetInstanceProcAddr");
        return false;
    }
    return true;
}

void unload_vulkan_loader() {
    if (vulkan_library) {
        FreeLibrary(vulkan_library);
    }
}
#elif defined(__ANDROID__)
#include <dlfcn.h>
static void* vulkan_library = NULL;

bool load_vulkan_loader() {
    vulkan_library = dlopen("libvulkan.so", RTLD_NOW | RTLD_LOCAL);
    if (!vulkan_library) {
        LOG_ERROR("Failed to load libvulkan.so: %s", dlerror());
        return false;
    }
    vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)dlsym(vulkan_library, "vkGetInstanceProcAddr");
    if (!vkGetInstanceProcAddr) {
        LOG_ERROR("Failed to get vkGetInstanceProcAddr");
        return false;
    }
    return true;
}

void unload_vulkan_loader() {
    if (vulkan_library) {
        dlclose(vulkan_library);
    }
}
#else // Linux
#include <dlfcn.h>
static void* vulkan_library = NULL;

bool load_vulkan_loader() {
    vulkan_library = dlopen("libvulkan.so.1", RTLD_NOW | RTLD_LOCAL);
    if (!vulkan_library) {
        LOG_ERROR("Failed to load libvulkan.so.1: %s", dlerror());
        return false;
    }
    vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)dlsym(vulkan_library, "vkGetInstanceProcAddr");
    if (!vkGetInstanceProcAddr) {
        LOG_ERROR("Failed to get vkGetInstanceProcAddr");
        return false;
    }
    return true;
}

void unload_vulkan_loader() {
    if (vulkan_library) {
        dlclose(vulkan_library);
    }
}
#endif

// ============================================
// LOAD VULKAN FUNCTIONS
// ============================================
#define LOAD_INSTANCE_FUNC(name) \
    name = (PFN_##name)vkGetInstanceProcAddr(vk_state.instance, #name); \
    if (!name) { LOG_ERROR("Failed to load " #name); return false; }

#define LOAD_DEVICE_FUNC(name) \
    name = (PFN_##name)vkGetDeviceProcAddr(vk_state.device, #name); \
    if (!name) { LOG_ERROR("Failed to load " #name); return false; }

bool load_global_functions() {
    vkCreateInstance = (PFN_vkCreateInstance)vkGetInstanceProcAddr(NULL, "vkCreateInstance");
    vkEnumerateInstanceExtensionProperties = (PFN_vkEnumerateInstanceExtensionProperties)vkGetInstanceProcAddr(NULL, "vkEnumerateInstanceExtensionProperties");
    vkEnumerateInstanceLayerProperties = (PFN_vkEnumerateInstanceLayerProperties)vkGetInstanceProcAddr(NULL, "vkEnumerateInstanceLayerProperties");

    if (!vkCreateInstance || !vkEnumerateInstanceExtensionProperties || !vkEnumerateInstanceLayerProperties) {
        LOG_ERROR("Failed to load global Vulkan functions");
        return false;
    }
    return true;
}

bool load_instance_functions() {
    LOAD_INSTANCE_FUNC(vkDestroyInstance);
    LOAD_INSTANCE_FUNC(vkEnumeratePhysicalDevices);
    LOAD_INSTANCE_FUNC(vkGetPhysicalDeviceProperties);
    LOAD_INSTANCE_FUNC(vkGetPhysicalDeviceFeatures);
    LOAD_INSTANCE_FUNC(vkGetPhysicalDeviceQueueFamilyProperties);
    LOAD_INSTANCE_FUNC(vkGetPhysicalDeviceSurfaceSupportKHR);
    LOAD_INSTANCE_FUNC(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
    LOAD_INSTANCE_FUNC(vkGetPhysicalDeviceSurfaceFormatsKHR);
    LOAD_INSTANCE_FUNC(vkGetPhysicalDeviceSurfacePresentModesKHR);
    LOAD_INSTANCE_FUNC(vkCreateDevice);
    LOAD_INSTANCE_FUNC(vkGetDeviceProcAddr);
    LOAD_INSTANCE_FUNC(vkDestroySurfaceKHR);
    LOAD_INSTANCE_FUNC(vkEnumerateDeviceExtensionProperties);

#if defined(_WIN32)
    LOAD_INSTANCE_FUNC(vkCreateWin32SurfaceKHR);
#elif defined(__ANDROID__)
    LOAD_INSTANCE_FUNC(vkCreateAndroidSurfaceKHR);
#elif defined(__linux__)
    LOAD_INSTANCE_FUNC(vkCreateXlibSurfaceKHR);
#endif

#ifdef DEBUG_VERBOSE
    LOAD_INSTANCE_FUNC(vkCreateDebugUtilsMessengerEXT);
    LOAD_INSTANCE_FUNC(vkDestroyDebugUtilsMessengerEXT);
#endif

    return true;
}

bool load_device_functions() {
    LOAD_DEVICE_FUNC(vkGetDeviceQueue);
    LOAD_DEVICE_FUNC(vkCreateSwapchainKHR);
    LOAD_DEVICE_FUNC(vkDestroySwapchainKHR);
    LOAD_DEVICE_FUNC(vkGetSwapchainImagesKHR);
    LOAD_DEVICE_FUNC(vkCreateImageView);
    LOAD_DEVICE_FUNC(vkDestroyImageView);
    LOAD_DEVICE_FUNC(vkCreateRenderPass);
    LOAD_DEVICE_FUNC(vkDestroyRenderPass);
    LOAD_DEVICE_FUNC(vkCreateShaderModule);
    LOAD_DEVICE_FUNC(vkDestroyShaderModule);
    LOAD_DEVICE_FUNC(vkCreatePipelineLayout);
    LOAD_DEVICE_FUNC(vkDestroyPipelineLayout);
    LOAD_DEVICE_FUNC(vkCreateGraphicsPipelines);
    LOAD_DEVICE_FUNC(vkDestroyPipeline);
    LOAD_DEVICE_FUNC(vkCreateFramebuffer);
    LOAD_DEVICE_FUNC(vkDestroyFramebuffer);
    LOAD_DEVICE_FUNC(vkCreateCommandPool);
    LOAD_DEVICE_FUNC(vkDestroyCommandPool);
    LOAD_DEVICE_FUNC(vkAllocateCommandBuffers);
    LOAD_DEVICE_FUNC(vkFreeCommandBuffers);
    LOAD_DEVICE_FUNC(vkBeginCommandBuffer);
    LOAD_DEVICE_FUNC(vkEndCommandBuffer);
    LOAD_DEVICE_FUNC(vkResetCommandBuffer);
    LOAD_DEVICE_FUNC(vkCmdBeginRenderPass);
    LOAD_DEVICE_FUNC(vkCmdEndRenderPass);
    LOAD_DEVICE_FUNC(vkCmdBindPipeline);
    LOAD_DEVICE_FUNC(vkCmdDraw);
    LOAD_DEVICE_FUNC(vkCreateSemaphore);
    LOAD_DEVICE_FUNC(vkDestroySemaphore);
    LOAD_DEVICE_FUNC(vkCreateFence);
    LOAD_DEVICE_FUNC(vkDestroyFence);
    LOAD_DEVICE_FUNC(vkWaitForFences);
    LOAD_DEVICE_FUNC(vkResetFences);
    LOAD_DEVICE_FUNC(vkAcquireNextImageKHR);
    LOAD_DEVICE_FUNC(vkQueueSubmit);
    LOAD_DEVICE_FUNC(vkQueuePresentKHR);
    LOAD_DEVICE_FUNC(vkDeviceWaitIdle);
    LOAD_DEVICE_FUNC(vkDestroyDevice);

    return true;
}

// ============================================
// VALIDATION LAYER CALLBACK
// ============================================
#ifdef DEBUG_VERBOSE
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data) {

    (void)type;
    (void)user_data;

    if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        LOG_WARN("[Vulkan] %s", callback_data->pMessage);
    }

    return VK_FALSE;
}
#endif

// ============================================
// TO BE CONTINUED...
// ============================================

// Forward declarations
bool create_instance();
bool create_surface();
bool pick_physical_device();
bool create_logical_device();
bool create_swapchain();
bool create_image_views();
bool create_render_pass();
bool create_graphics_pipeline();
bool create_framebuffers();
bool create_command_pool();
bool create_command_buffers();
bool create_sync_objects();
void record_command_buffer(VkCommandBuffer cmd_buffer, uint32_t image_index);

// Cleanup functions
void cleanup_swapchain();
void cleanup_vulkan();

// sokol_app callbacks
static void init(void);
static void frame(void);
static void cleanup(void);
static void input_event(const sapp_event* e);

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
        .window_title = "Vulkan Media Player - Native Implementation",
        .logger.func = slog_func,
    };
}

// ============================================
// VULKAN INSTANCE CREATION
// ============================================
bool create_instance() {
    LOG_INFO("Creating Vulkan instance...");

    VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Vulkan Media Player",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_0,
    };

    // Required extensions
    const char* extensions[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
#if defined(_WIN32)
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#elif defined(__ANDROID__)
        VK_KHR_ANDROID_SURFACE_EXTENSION_NAME,
#elif defined(__linux__)
        VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
#endif
#ifdef DEBUG_VERBOSE
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
    };

#ifdef DEBUG_VERBOSE
    const char* validation_layers[] = {
        "VK_LAYER_KHRONOS_validation"
    };
    uint32_t layer_count = 1;
#else
    const char** validation_layers = NULL;
    uint32_t layer_count = 0;
#endif

    VkInstanceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
        .enabledExtensionCount = sizeof(extensions) / sizeof(extensions[0]),
        .ppEnabledExtensionNames = extensions,
        .enabledLayerCount = layer_count,
        .ppEnabledLayerNames = validation_layers,
    };

    VK_CHECK(vkCreateInstance(&create_info, NULL, &vk_state.instance));
    LOG_SUCCESS("Vulkan instance created");

    return true;
}

// ============================================
// SURFACE CREATION (Platform-specific)
// ============================================
bool create_surface() {
    LOG_INFO("Creating Vulkan surface...");

#if defined(_WIN32)
    VkWin32SurfaceCreateInfoKHR create_info = {
        .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .hinstance = GetModuleHandle(NULL),
        .hwnd = (HWND)sapp_win32_get_hwnd(),
    };
    VK_CHECK(vkCreateWin32SurfaceKHR(vk_state.instance, &create_info, NULL, &vk_state.surface));

#elif defined(__ANDROID__)
    VkAndroidSurfaceCreateInfoKHR create_info = {
        .sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,
        .window = (ANativeWindow*)sapp_android_get_native_activity()->window,
    };
    VK_CHECK(vkCreateAndroidSurfaceKHR(vk_state.instance, &create_info, NULL, &vk_state.surface));

#elif defined(__linux__)
    VkXlibSurfaceCreateInfoKHR create_info = {
        .sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
        .dpy = sapp_x11_get_display(),
        .window = sapp_x11_get_window(),
    };
    VK_CHECK(vkCreateXlibSurfaceKHR(vk_state.instance, &create_info, NULL, &vk_state.surface));
#endif

    LOG_SUCCESS("Vulkan surface created");
    return true;
}

// ============================================
// PHYSICAL DEVICE SELECTION
// ============================================
bool pick_physical_device() {
    LOG_INFO("Selecting physical device...");

    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(vk_state.instance, &device_count, NULL);

    if (device_count == 0) {
        LOG_ERROR("Failed to find GPUs with Vulkan support");
        return false;
    }

    VkPhysicalDevice* devices = malloc(sizeof(VkPhysicalDevice) * device_count);
    vkEnumeratePhysicalDevices(vk_state.instance, &device_count, devices);

    // Pick first device (simplification - in production you'd score devices)
    vk_state.physical_device = devices[0];
    free(devices);

    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(vk_state.physical_device, &properties);
    LOG_SUCCESS("Selected GPU: %s", properties.deviceName);
    LOG_DEBUG("  - API Version: %d.%d.%d",
        VK_VERSION_MAJOR(properties.apiVersion),
        VK_VERSION_MINOR(properties.apiVersion),
        VK_VERSION_PATCH(properties.apiVersion));

    return true;
}

// ============================================
// FIND QUEUE FAMILIES
// ============================================
bool find_queue_families() {
    LOG_INFO("Finding queue families...");

    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(vk_state.physical_device, &queue_family_count, NULL);

    VkQueueFamilyProperties* queue_families = malloc(sizeof(VkQueueFamilyProperties) * queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(vk_state.physical_device, &queue_family_count, queue_families);

    bool graphics_found = false;
    bool present_found = false;

    for (uint32_t i = 0; i < queue_family_count; i++) {
        if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            vk_state.graphics_family_index = i;
            graphics_found = true;
        }

        VkBool32 present_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(vk_state.physical_device, i, vk_state.surface, &present_support);
        if (present_support) {
            vk_state.present_family_index = i;
            present_found = true;
        }

        if (graphics_found && present_found) {
            break;
        }
    }

    free(queue_families);

    if (!graphics_found || !present_found) {
        LOG_ERROR("Failed to find suitable queue families");
        return false;
    }

    LOG_SUCCESS("Queue families found (graphics=%d, present=%d)",
        vk_state.graphics_family_index, vk_state.present_family_index);

    return true;
}

// ============================================
// LOGICAL DEVICE CREATION
// ============================================
bool create_logical_device() {
    LOG_INFO("Creating logical device...");

    if (!find_queue_families()) {
        return false;
    }

    float queue_priority = 1.0f;
    VkDeviceQueueCreateInfo queue_create_infos[2];
    uint32_t queue_create_info_count = 0;

    // Graphics queue
    queue_create_infos[0] = (VkDeviceQueueCreateInfo){
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = vk_state.graphics_family_index,
        .queueCount = 1,
        .pQueuePriorities = &queue_priority,
    };
    queue_create_info_count++;

    // Present queue (if different from graphics)
    if (vk_state.present_family_index != vk_state.graphics_family_index) {
        queue_create_infos[1] = (VkDeviceQueueCreateInfo){
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = vk_state.present_family_index,
            .queueCount = 1,
            .pQueuePriorities = &queue_priority,
        };
        queue_create_info_count++;
    }

    VkPhysicalDeviceFeatures device_features = {0};

    const char* device_extensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };

    VkDeviceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = queue_create_info_count,
        .pQueueCreateInfos = queue_create_infos,
        .pEnabledFeatures = &device_features,
        .enabledExtensionCount = 1,
        .ppEnabledExtensionNames = device_extensions,
    };

    VK_CHECK(vkCreateDevice(vk_state.physical_device, &create_info, NULL, &vk_state.device));
    LOG_SUCCESS("Logical device created");

    return true;
}

// ============================================
// GET DEVICE QUEUES
// ============================================
bool get_device_queues() {
    vkGetDeviceQueue(vk_state.device, vk_state.graphics_family_index, 0, &vk_state.graphics_queue);
    vkGetDeviceQueue(vk_state.device, vk_state.present_family_index, 0, &vk_state.present_queue);
    LOG_SUCCESS("Device queues retrieved");
    return true;
}

// ============================================
// INCLUDE IMPLEMENTATION PARTS
// ============================================
#include "vulkan_impl.h"
#include "vulkan_render.h"
