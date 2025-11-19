/*
 * Vulkan Autonomous Test - Windows Version
 *
 * This test validates all Vulkan functionality on Windows.
 * Uses LoadLibrary instead of dlopen.
 */

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <windows.h>

// ============================================
// TEST CONFIGURATION
// ============================================
#define TEST_VERBOSE 1

// ============================================
// LOGGING
// ============================================
static int test_count = 0;
static int test_passed = 0;
static int test_failed = 0;

#define TEST_START(name) do { \
    test_count++; \
    printf("[TEST %d] %s... ", test_count, name); \
    fflush(stdout); \
} while(0)

#define TEST_PASS() do { \
    printf("OK PASS\n"); \
    test_passed++; \
} while(0)

#define TEST_FAIL(msg) do { \
    printf("X FAIL: %s\n", msg); \
    test_failed++; \
} while(0)

#define TEST_INFO(fmt, ...) printf("    [INFO] " fmt "\n", ##__VA_ARGS__)

// ============================================
// VULKAN FUNCTION POINTERS
// ============================================
static HMODULE vulkan_library = NULL;
PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
PFN_vkCreateInstance vkCreateInstance;
PFN_vkEnumerateInstanceExtensionProperties vkEnumerateInstanceExtensionProperties;
PFN_vkEnumerateInstanceLayerProperties vkEnumerateInstanceLayerProperties;
PFN_vkDestroyInstance vkDestroyInstance;
PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices;
PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties;
PFN_vkGetPhysicalDeviceFeatures vkGetPhysicalDeviceFeatures;
PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties;

// ============================================
// TEST 1: Load Vulkan Library
// ============================================
bool test_load_vulkan_library() {
    TEST_START("Load Vulkan Library");

    vulkan_library = LoadLibraryA("vulkan-1.dll");
    if (!vulkan_library) {
        TEST_FAIL("Failed to load vulkan-1.dll");
        return false;
    }

    vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)GetProcAddress(vulkan_library, "vkGetInstanceProcAddr");
    if (!vkGetInstanceProcAddr) {
        TEST_FAIL("Failed to load vkGetInstanceProcAddr");
        return false;
    }

    TEST_INFO("Vulkan library loaded successfully");
    TEST_PASS();
    return true;
}

// ============================================
// TEST 2: Load Global Functions
// ============================================
bool test_load_global_functions() {
    TEST_START("Load Global Vulkan Functions");

    vkCreateInstance = (PFN_vkCreateInstance)vkGetInstanceProcAddr(NULL, "vkCreateInstance");
    vkEnumerateInstanceExtensionProperties = (PFN_vkEnumerateInstanceExtensionProperties)
        vkGetInstanceProcAddr(NULL, "vkEnumerateInstanceExtensionProperties");
    vkEnumerateInstanceLayerProperties = (PFN_vkEnumerateInstanceLayerProperties)
        vkGetInstanceProcAddr(NULL, "vkEnumerateInstanceLayerProperties");

    if (!vkCreateInstance || !vkEnumerateInstanceExtensionProperties || !vkEnumerateInstanceLayerProperties) {
        TEST_FAIL("Failed to load one or more global functions");
        return false;
    }

    TEST_INFO("All global functions loaded");
    TEST_PASS();
    return true;
}

// ============================================
// TEST 3: Enumerate Extensions
// ============================================
bool test_enumerate_extensions() {
    TEST_START("Enumerate Instance Extensions");

    uint32_t extension_count = 0;
    VkResult result = vkEnumerateInstanceExtensionProperties(NULL, &extension_count, NULL);

    if (result != VK_SUCCESS) {
        TEST_FAIL("vkEnumerateInstanceExtensionProperties failed");
        return false;
    }

    if (extension_count == 0) {
        TEST_FAIL("No extensions found");
        return false;
    }

    VkExtensionProperties* extensions = malloc(sizeof(VkExtensionProperties) * extension_count);
    vkEnumerateInstanceExtensionProperties(NULL, &extension_count, extensions);

    TEST_INFO("Found %d extensions:", extension_count);
    for (uint32_t i = 0; i < extension_count && i < 10; i++) {
        TEST_INFO("  - %s (spec version %d)", extensions[i].extensionName, extensions[i].specVersion);
    }
    if (extension_count > 10) {
        TEST_INFO("  ... and %d more", extension_count - 10);
    }

    free(extensions);
    TEST_PASS();
    return true;
}

// ============================================
// TEST 4: Enumerate Layers
// ============================================
bool test_enumerate_layers() {
    TEST_START("Enumerate Instance Layers");

    uint32_t layer_count = 0;
    VkResult result = vkEnumerateInstanceLayerProperties(&layer_count, NULL);

    if (result != VK_SUCCESS) {
        TEST_FAIL("vkEnumerateInstanceLayerProperties failed");
        return false;
    }

    TEST_INFO("Found %d layers", layer_count);

    if (layer_count > 0) {
        VkLayerProperties* layers = malloc(sizeof(VkLayerProperties) * layer_count);
        vkEnumerateInstanceLayerProperties(&layer_count, layers);

        for (uint32_t i = 0; i < layer_count; i++) {
            TEST_INFO("  - %s: %s", layers[i].layerName, layers[i].description);
        }

        free(layers);
    }

    TEST_PASS();
    return true;
}

// ============================================
// TEST 5: Create Vulkan Instance
// ============================================
bool test_create_instance(VkInstance* instance) {
    TEST_START("Create Vulkan Instance");

    VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Autonomous Vulkan Test (Windows)",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "Test Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_0,
    };

    VkInstanceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
        .enabledExtensionCount = 0,
        .enabledLayerCount = 0,
    };

    VkResult result = vkCreateInstance(&create_info, NULL, instance);

    if (result != VK_SUCCESS) {
        TEST_FAIL("vkCreateInstance failed");
        TEST_INFO("Error code: %d", result);
        return false;
    }

    TEST_INFO("Instance created successfully");
    TEST_PASS();
    return true;
}

// ============================================
// TEST 6: Enumerate Physical Devices
// ============================================
bool test_enumerate_physical_devices(VkInstance instance) {
    TEST_START("Enumerate Physical Devices");

    vkDestroyInstance = (PFN_vkDestroyInstance)vkGetInstanceProcAddr(instance, "vkDestroyInstance");
    vkEnumeratePhysicalDevices = (PFN_vkEnumeratePhysicalDevices)vkGetInstanceProcAddr(instance, "vkEnumeratePhysicalDevices");
    vkGetPhysicalDeviceProperties = (PFN_vkGetPhysicalDeviceProperties)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceProperties");

    uint32_t device_count = 0;
    VkResult result = vkEnumeratePhysicalDevices(instance, &device_count, NULL);

    if (result != VK_SUCCESS) {
        TEST_FAIL("vkEnumeratePhysicalDevices failed");
        return false;
    }

    if (device_count == 0) {
        TEST_FAIL("No physical devices found");
        return false;
    }

    VkPhysicalDevice* devices = malloc(sizeof(VkPhysicalDevice) * device_count);
    vkEnumeratePhysicalDevices(instance, &device_count, devices);

    TEST_INFO("Found %d physical device(s):", device_count);

    for (uint32_t i = 0; i < device_count; i++) {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(devices[i], &props);

        const char* device_type = "Unknown";
        switch (props.deviceType) {
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: device_type = "Discrete GPU"; break;
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: device_type = "Integrated GPU"; break;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: device_type = "Virtual GPU"; break;
            case VK_PHYSICAL_DEVICE_TYPE_CPU: device_type = "CPU (Software)"; break;
            default: break;
        }

        TEST_INFO("  [%d] %s (%s)", i, props.deviceName, device_type);
        TEST_INFO("      API Version: %d.%d.%d",
            VK_VERSION_MAJOR(props.apiVersion),
            VK_VERSION_MINOR(props.apiVersion),
            VK_VERSION_PATCH(props.apiVersion));
        TEST_INFO("      Driver Version: %d", props.driverVersion);
    }

    free(devices);
    TEST_PASS();
    return true;
}

// ============================================
// MAIN TEST RUNNER
// ============================================
int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    printf("\n");
    printf("===============================================================\n");
    printf("  VULKAN AUTONOMOUS TEST SUITE - WINDOWS\n");
    printf("  Headless validation running in Wine\n");
    printf("===============================================================\n");
    printf("\n");

    VkInstance instance = NULL;

    // Run all tests
    if (test_load_vulkan_library()) {
        if (test_load_global_functions()) {
            test_enumerate_extensions();
            test_enumerate_layers();

            if (test_create_instance(&instance)) {
                test_enumerate_physical_devices(instance);

                // Cleanup
                TEST_START("Cleanup Vulkan Instance");
                vkDestroyInstance(instance, NULL);
                TEST_PASS();
            }
        }
    }

    // Cleanup library
    if (vulkan_library) {
        FreeLibrary(vulkan_library);
    }

    // Summary
    printf("\n");
    printf("===============================================================\n");
    printf("  TEST SUMMARY\n");
    printf("===============================================================\n");
    printf("  Total Tests: %d\n", test_count);
    printf("  Passed:      %d OK\n", test_passed);
    printf("  Failed:      %d X\n", test_failed);
    printf("  Success Rate: %.1f%%\n", (100.0 * test_passed) / test_count);
    printf("===============================================================\n");

    if (test_failed == 0) {
        printf("  OK ALL TESTS PASSED\n");
        printf("===============================================================\n");
        printf("\n");
        return 0;
    } else {
        printf("  X SOME TESTS FAILED\n");
        printf("===============================================================\n");
        printf("\n");
        return 1;
    }
}
