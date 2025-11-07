cmake_minimum_required(VERSION 3.14)
project(simple_media_server CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable FetchContent for downloading dependencies
include(FetchContent)

# Download cpp-httplib (HTTP server with range request support)
FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.14.3
)

# Download nlohmann/json (JSON parsing)
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)

FetchContent_MakeAvailable(httplib json)

# Source files
add_executable(media_server
    main.cpp
    scanner.cpp
    video_info.cpp
)

# Include directories
target_include_directories(media_server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Link libraries
target_link_libraries(media_server PRIVATE httplib::httplib nlohmann_json::nlohmann_json)

# Enable threading support
find_package(Threads REQUIRED)
target_link_libraries(media_server PRIVATE Threads::Threads)

# Compiler warnings
if(MSVC)
    target_compile_options(media_server PRIVATE /W4)
else()
    target_compile_options(media_server PRIVATE -Wall -Wextra -pedantic)
endif()
