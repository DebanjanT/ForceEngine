# External dependencies for Force Engine

# GLFW - Window management
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(glfw)

# GLM - Math library (header-only)
add_subdirectory(glm)

# spdlog - Logging
add_subdirectory(spdlog)

# VMA - Vulkan Memory Allocator (header-only, we'll create an interface target)
add_library(VulkanMemoryAllocator INTERFACE)
target_include_directories(VulkanMemoryAllocator INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/VulkanMemoryAllocator/include)

# Tracy - Profiling (optional)
if(FORCE_ENABLE_PROFILING)
    option(TRACY_ENABLE "" ON)
    option(TRACY_ON_DEMAND "" ON)
    add_subdirectory(tracy)
endif()

# EnTT - Entity Component System (header-only)
add_library(EnTT INTERFACE)
target_include_directories(EnTT INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/entt/single_include)

# stb - Image loading (header-only)
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/stb)

# Assimp - Asset import library
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_FBX_IMPORTER ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_GLTF_IMPORTER ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_COLLADA_IMPORTER ON CACHE BOOL "" FORCE)
add_subdirectory(assimp)

# nlohmann/json - JSON serialization (header-only)
set(JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory(json)

# Jolt Physics - rigid body simulation
set(JPH_ENABLE_INSTALL                  OFF CACHE BOOL "" FORCE)
set(JPH_BUILD_UNIT_TESTS                OFF CACHE BOOL "" FORCE)
set(JPH_BUILD_PERFORMANCE_TEST          OFF CACHE BOOL "" FORCE)
set(USE_STATIC_MSVC_RUNTIME_LIBRARY     OFF CACHE BOOL "" FORCE)
add_subdirectory(JoltPhysics/Build)

# OpenAL Soft - 3D positional audio
set(ALSOFT_EXAMPLES  OFF CACHE BOOL "" FORCE)
set(ALSOFT_TESTS     OFF CACHE BOOL "" FORCE)
set(ALSOFT_INSTALL   OFF CACHE BOOL "" FORCE)
set(ALSOFT_UTILS     OFF CACHE BOOL "" FORCE)
add_subdirectory(openal-soft)

# Dear ImGui - Immediate mode GUI
add_library(imgui STATIC
    imgui/imgui.cpp
    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/backends/imgui_impl_glfw.cpp
    imgui/backends/imgui_impl_vulkan.cpp
)
target_include_directories(imgui PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends
)
target_link_libraries(imgui PUBLIC glfw Vulkan::Vulkan)

# ImGuizmo - Gizmo manipulation (optional)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ImGuizmo/ImGuizmo.h)
    add_library(ImGuizmo STATIC
        ImGuizmo/ImGuizmo.cpp
    )
    target_include_directories(ImGuizmo PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/ImGuizmo)
    target_link_libraries(ImGuizmo PUBLIC imgui)
endif()

# implot - Plotting widgets (optional)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/implot/implot.h)
    add_library(implot STATIC
        implot/implot.cpp
        implot/implot_items.cpp
    )
    target_include_directories(implot PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/implot)
    target_link_libraries(implot PUBLIC imgui)
endif()

# imnodes - Node editor (optional)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/imnodes/imnodes.h)
    add_library(imnodes STATIC
        imnodes/imnodes.cpp
    )
    target_include_directories(imnodes PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/imnodes)
    target_link_libraries(imnodes PUBLIC imgui)
endif()
