# Vulkan Media Player - Native Implementation

**🎯 Maximum Versatility Cross-Platform Media Player**

Pure Vulkan API rendering + sokol_app.h windowing for complete hardware control.

---

## 🌟 Why Vulkan?

**Máxima versatilidad y escalabilidad** según lo solicitado:

✅ **Control Total del Hardware**
- Acceso directo a GPU features
- Custom extensions (ray tracing, mesh shaders, etc.)
- Multi-GPU support
- Explicit memory management

✅ **Future-Proof**
- Cualquier nueva extensión Vulkan es soportada
- No limitado por abstracciones de terceros
- Hardware custom → solo necesitas el driver Vulkan

✅ **Cross-Platform**
- **Linux**: VK_KHR_xlib_surface
- **Windows**: VK_KHR_win32_surface
- **Android**: VK_KHR_android_surface

---

## 📦 Arquitectura

```
Windowing & Input: sokol_app.h (solo ventanas, NO gráficos)
                        ↓
Graphics Rendering: PURE VULKAN API
                        ↓
        ┌────────────┬──────────┬────────────┐
        ↓            ↓          ↓            ↓
    Instance    Surface    Device      Swapchain
        ↓            ↓          ↓            ↓
    Physical    Queue      Pipeline   Rendering
```

### Componentes

1. **sokol_app.h** - SOLO window management
   - Abstracción de ventanas multiplataforma
   - Input events (teclado, mouse, touch)
   - **NO SE USA** para rendering (backend dummy)

2. **Vulkan API** - 100% control de rendering
   - Dynamic function loading
   - Platform-specific surfaces
   - Custom pipeline configuration

---

## 🏗️ Build Instructions

### Linux

```bash
# Prerequisites
sudo apt-get install libvulkan-dev libx11-dev libxi-dev \
  libxcursor-dev libgl-dev libasound-dev

# Build
mkdir build && cd build
cmake ..
make

# Run (requires Vulkan drivers)
./vulkan_media_player
```

**Drivers needed**:
- NVIDIA: `nvidia-driver`
- AMD/Intel: `mesa-vulkan-drivers`

### Windows

```bash
# Prerequisites: Vulkan SDK from LunarG

mkdir build && cd build
cmake ..
cmake --build .

# Run
vulkan_media_player.exe
```

### Android

```bash
cd android
./gradlew assembleDebug
adb install app/build/outputs/apk/debug/app-debug.apk
```

**Requiere**: Android 7.0+ (API 24) con GPU Vulkan-capable

---

## 📂 Source Code Structure

```
sokol-native/
├── src/
│   ├── vulkan_main.c      # Vulkan loader, instance, device (686 lines)
│   ├── vulkan_impl.h      # Swapchain, shaders, helpers (330 lines)
│   └── vulkan_render.h    # Pipeline, rendering loop (500 lines)
├── external/sokol/        # sokol headers (windowing only)
├── CMakeLists.txt         # Cross-platform build
├── android/               # Android Gradle project
└── VULKAN_BUILD_RESULTS.md # Complete documentation
```

**Total**: ~1900 lines of Vulkan implementation

---

## 🔧 Key Features Implemented

### Vulkan Core
- ✅ VkInstance with validation layers
- ✅ VkPhysicalDevice selection
- ✅ VkDevice + Queue families (graphics + present)
- ✅ Platform-specific VkSurfaceKHR creation

### Rendering Pipeline
- ✅ VkSwapchainKHR (optimal format/present mode)
- ✅ VkRenderPass (color attachment)
- ✅ VkShaderModule (SPIR-V hardcoded shaders)
- ✅ VkGraphicsPipeline (complete configuration)
- ✅ VkFramebuffer per swapchain image

### Command Recording
- ✅ VkCommandPool
- ✅ VkCommandBuffer allocation and recording
- ✅ Proper synchronization (VkSemaphore, VkFence)
- ✅ Acquire → Render → Present flow

### Dynamic Loading
- ✅ All Vulkan functions loaded via `dlopen`/`LoadLibrary`
- ✅ No static SDK dependency
- ✅ `vkGetInstanceProcAddr` / `vkGetDeviceProcAddr`

---

## 🎨 Rendering Flow

```c
1. Wait for previous frame (vkWaitForFences)
2. Acquire next image (vkAcquireNextImageKHR)
3. Record command buffer:
   - Begin render pass
   - Bind pipeline
   - Draw geometry
   - End render pass
4. Submit to queue (vkQueueSubmit)
5. Present image (vkQueuePresentKHR)
```

---

## 🚀 Extensibility Examples

### Add Ray Tracing

```c
// 1. Check extension support
VkPhysicalDeviceRayTracingPipelinePropertiesKHR rt_props = {...};

// 2. Enable extension
const char* device_extensions[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
    VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
};

// 3. Load functions
vkCreateRayTracingPipelinesKHR = vkGetDeviceProcAddr(...);

// 4. Use custom ray tracing pipeline
```

### Multi-GPU Support

```c
// Enumerate all GPUs
uint32_t device_count;
vkEnumeratePhysicalDevices(instance, &device_count, NULL);
VkPhysicalDevice* devices = malloc(sizeof(VkPhysicalDevice) * device_count);
vkEnumeratePhysicalDevices(instance, &device_count, devices);

// Choose best GPU or use multiple
for (uint32_t i = 0; i < device_count; i++) {
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(devices[i], &props);

    if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        // Prefer discrete GPU
    }
}
```

### Custom Memory Pools

```c
// Find optimal memory type
uint32_t find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties mem_properties;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_properties);

    for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
        if ((type_filter & (1 << i)) &&
            (mem_properties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
}

// Allocate custom memory
VkMemoryAllocateInfo alloc_info = {
    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    .allocationSize = requirements.size,
    .memoryTypeIndex = find_memory_type(requirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
};
vkAllocateMemory(device, &alloc_info, NULL, &memory);
```

---

## 📊 Comparison with Previous Implementation

| Aspect | sokol_gfx (OpenGL) | Pure Vulkan |
|--------|-------------------|-------------|
| **Control Level** | High-level abstraction | Low-level, explicit |
| **Hardware Access** | Limited | Complete |
| **Extensions** | Through sokol | Direct Vulkan extensions |
| **Memory Management** | Automatic | Manual (optimizable) |
| **Multi-GPU** | No | Yes |
| **Future Extensions** | Depends on sokol updates | Immediate via Vulkan |
| **Code Size** | ~500 lines | ~1900 lines |
| **Binary Size** | 280KB (OpenGL) | 177KB (Vulkan!) |
| **Versatility** | ⭐⭐⭐ | ⭐⭐⭐⭐⭐ |

---

## 🐛 Troubleshooting

### "Could not find Vulkan drivers"

```bash
# Check Vulkan installation
vulkaninfo

# Install drivers
# NVIDIA:
sudo apt-get install nvidia-driver

# AMD/Intel:
sudo apt-get install mesa-vulkan-drivers
```

### "No suitable GPU found"

Your GPU might not support Vulkan. Check:
```bash
vkEnumeratePhysicalDevices returned 0 devices
```

Requirements:
- NVIDIA: GeForce 600 series or newer
- AMD: GCN 1.0 or newer
- Intel: Ivy Bridge or newer (Linux), Skylake+ (Windows)

### Headless/CI Environment

Vulkan requires GPU drivers. For software rendering:
```bash
# Install SwiftShader (CPU-based Vulkan)
sudo apt-get install libvulkan1 mesa-vulkan-drivers swiftshader
```

---

## 📚 Documentation

- **VULKAN_BUILD_RESULTS.md** - Complete build results and analysis
- **ARCHITECTURE.md** - Original Sokol architecture (historical)
- **DEBUG_AUTONOMOUS.md** - Autonomous debugging methodology

---

## 🔗 References

- [Vulkan Tutorial](https://vulkan-tutorial.com/)
- [Vulkan Spec](https://www.vulkan.org/specs/)
- [sokol headers](https://github.com/floooh/sokol)
- [Vulkan SDK](https://vulkan.lunarg.com/)

---

## 🎯 Summary

**Implementación completa de Vulkan nativo** para máxima versatilidad:

✅ ~1900 líneas de código Vulkan puro
✅ Soporte para Linux, Windows, Android
✅ Control total sobre hardware y GPU
✅ Extensible para ray tracing, mesh shaders, etc.
✅ Sin abstracciones limitantes
✅ Dynamic function loading
✅ Compilación exitosa en Linux (177KB)

**Perfecto para**: Aplicaciones que necesitan máximo control del hardware, soporte para extensiones custom, y escalabilidad futura.

---

Desarrollado con control total sobre el rendering pipeline 🚀
