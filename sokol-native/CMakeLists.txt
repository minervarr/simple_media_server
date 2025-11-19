cmake_minimum_required(VERSION 3.14)
project(sokol_media_player C)

set(CMAKE_C_STANDARD 11)

# Platform detection
if(WIN32)
    set(SOKOL_BACKEND "SOKOL_D3D11")
    set(LIBS d3d11 dxgi)
elseif(APPLE)
    set(SOKOL_BACKEND "SOKOL_METAL")
    set(LIBS "-framework Cocoa" "-framework QuartzCore" "-framework Metal" "-framework MetalKit" "-framework AudioToolbox")
elseif(ANDROID)
    set(SOKOL_BACKEND "SOKOL_GLES3")
    set(LIBS GLESv3 EGL android log)
else() # Linux
    set(SOKOL_BACKEND "SOKOL_GLCORE33")
    find_package(X11 REQUIRED)
    find_package(Threads REQUIRED)
    set(LIBS X11 Xi Xcursor GL asound dl m pthread)
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/external/sokol
)

# Source files
set(SOURCES
    src/main.c
)

# Executable
add_executable(sokol_media_player ${SOURCES})

# Platform-specific compiler flags
if(MSVC)
    target_compile_options(sokol_media_player PRIVATE /W3)
else()
    target_compile_options(sokol_media_player PRIVATE -Wall -Wextra)
endif()

# Link libraries
target_link_libraries(sokol_media_player ${LIBS})

# Install target
install(TARGETS sokol_media_player DESTINATION bin)

# Copy resources if any
# install(DIRECTORY assets/ DESTINATION bin/assets)

message(STATUS "Sokol Backend: ${SOKOL_BACKEND}")
message(STATUS "Platform Libraries: ${LIBS}")
