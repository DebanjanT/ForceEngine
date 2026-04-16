cmake_minimum_required(VERSION 3.21)
project(ForceEngine VERSION 0.1.0 LANGUAGES CXX C)

# C++20 Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# Options
option(FORCE_BUILD_EDITOR "Build the editor executable" ON)
option(FORCE_BUILD_RUNTIME "Build the runtime executable" ON)
option(FORCE_ENABLE_PROFILING "Enable Tracy profiling" ON)
option(FORCE_ENABLE_VALIDATION "Enable Vulkan validation layers" ON)

# Platform detection
if(WIN32)
    add_compile_definitions(FORCE_PLATFORM_WINDOWS)
elseif(UNIX AND NOT APPLE)
    add_compile_definitions(FORCE_PLATFORM_LINUX)
elseif(APPLE)
    add_compile_definitions(FORCE_PLATFORM_MACOS)
endif()

# Build configuration defines
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(FORCE_DEBUG)
endif()

if(FORCE_ENABLE_PROFILING)
    add_compile_definitions(FORCE_PROFILING_ENABLED TRACY_ENABLE)
endif()

if(FORCE_ENABLE_VALIDATION)
    add_compile_definitions(FORCE_VULKAN_VALIDATION)
endif()

# Find Vulkan
find_package(Vulkan REQUIRED)

# Add external dependencies
add_subdirectory(External)

# Add engine library
add_subdirectory(Engine)

# Add editor executable
if(FORCE_BUILD_EDITOR)
    add_subdirectory(Editor)
endif()

# Add runtime executable
if(FORCE_BUILD_RUNTIME)
    add_subdirectory(Runtime)
endif()

# Print configuration summary
message(STATUS "")
message(STATUS "=== Force Engine Configuration ===")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Editor: ${FORCE_BUILD_EDITOR}")
message(STATUS "Build Runtime: ${FORCE_BUILD_RUNTIME}")
message(STATUS "Profiling: ${FORCE_ENABLE_PROFILING}")
message(STATUS "Vulkan Validation: ${FORCE_ENABLE_VALIDATION}")
message(STATUS "==================================")
message(STATUS "")
