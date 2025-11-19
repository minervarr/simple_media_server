cmake_minimum_required(VERSION 3.14)
project(vulkan_media_player C)

set(CMAKE_C_STANDARD 11)

# Find Vulkan
find_package(Vulkan REQUIRED)

# Platform-specific libraries (excluding graphics backends, using Vulkan only)
if(WIN32)
    # Windows: Vulkan + audio + system
    set(PLATFORM_LIBS ${Vulkan_LIBRARIES})
elseif(APPLE)
    # macOS: Vulkan (MoltenVK) + frameworks
    set(PLATFORM_LIBS ${Vulkan_LIBRARIES} "-framework Cocoa" "-framework QuartzCore" "-framework AudioToolbox")
elseif(ANDROID)
    # Android: Vulkan + system
    set(PLATFORM_LIBS ${Vulkan_LIBRARIES} android log)
else() # Linux
    # Linux: Vulkan + X11 + audio + threading
    # Note: GL is needed by sokol_app.h for context creation (even though we use Vulkan for rendering)
    find_package(X11 REQUIRED)
    find_package(Threads REQUIRED)
    set(PLATFORM_LIBS ${Vulkan_LIBRARIES} X11 Xi Xcursor GL asound dl m pthread)
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/external/sokol
    ${Vulkan_INCLUDE_DIRS}
)

# Source files
set(SOURCES
    src/vulkan_main.c
)

# Executable
add_executable(vulkan_media_player ${SOURCES})

# Platform-specific compiler flags
if(MSVC)
    target_compile_options(vulkan_media_player PRIVATE /W3)
else()
    target_compile_options(vulkan_media_player PRIVATE -Wall -Wextra)
endif()

# Link libraries
target_link_libraries(vulkan_media_player ${PLATFORM_LIBS})

# Install target
install(TARGETS vulkan_media_player DESTINATION bin)

message(STATUS "Vulkan Include: ${Vulkan_INCLUDE_DIRS}")
message(STATUS "Vulkan Libraries: ${Vulkan_LIBRARIES}")
message(STATUS "Platform Libraries: ${PLATFORM_LIBS}")
