# Force Engine Core Library

set(ENGINE_SOURCES
    # Core
    Source/Core/Application.cpp
    Source/Core/Engine.cpp
    Source/Core/Logger.cpp
    Source/Core/Memory/Allocators.cpp
    Source/Core/Subsystem.cpp
    Source/Core/Event.cpp
    Source/Core/ECS.cpp
    Source/Core/Module.cpp
    Source/Core/CVar.cpp
    
    # Platform
    Source/Platform/Window.cpp
    Source/Platform/Platform.cpp
    Source/Platform/Input.cpp
    
    # Renderer
    Source/Renderer/VulkanContext.cpp
    Source/Renderer/VulkanDevice.cpp
    Source/Renderer/VulkanSwapchain.cpp
    Source/Renderer/VulkanPipeline.cpp
    Source/Renderer/VulkanBuffer.cpp
    Source/Renderer/VulkanCommandBuffer.cpp
    Source/Renderer/Renderer.cpp
)

set(ENGINE_HEADERS
    # Core
    Include/Force/Core/Base.h
    Include/Force/Core/Application.h
    Include/Force/Core/Engine.h
    Include/Force/Core/Logger.h
    Include/Force/Core/Assert.h
    Include/Force/Core/Types.h
    Include/Force/Core/Memory/Allocators.h
    Include/Force/Core/Memory/Memory.h
    Include/Force/Core/Subsystem.h
    Include/Force/Core/Event.h
    Include/Force/Core/ECS.h
    Include/Force/Core/Module.h
    Include/Force/Core/CVar.h
    
    # Platform
    Include/Force/Platform/Window.h
    Include/Force/Platform/Platform.h
    Include/Force/Platform/Input.h
    Include/Force/Platform/KeyCodes.h
    
    # Renderer
    Include/Force/Renderer/VulkanContext.h
    Include/Force/Renderer/VulkanDevice.h
    Include/Force/Renderer/VulkanSwapchain.h
    Include/Force/Renderer/VulkanPipeline.h
    Include/Force/Renderer/VulkanBuffer.h
    Include/Force/Renderer/VulkanCommandBuffer.h
    Include/Force/Renderer/Renderer.h
    Include/Force/Renderer/RendererTypes.h
    
    # Math
    Include/Force/Math/Math.h
)

add_library(ForceEngine STATIC ${ENGINE_SOURCES} ${ENGINE_HEADERS})

target_include_directories(ForceEngine 
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/Include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Source
)

target_link_libraries(ForceEngine 
    PUBLIC
        Vulkan::Vulkan
        glfw
        glm::glm
        spdlog::spdlog
        VulkanMemoryAllocator
        EnTT
)

if(FORCE_ENABLE_PROFILING)
    target_link_libraries(ForceEngine PUBLIC Tracy::TracyClient)
endif()

# Precompiled header
target_precompile_headers(ForceEngine PRIVATE Include/Force/Core/Base.h)

# Platform-specific settings
if(WIN32)
    target_compile_definitions(ForceEngine PRIVATE VK_USE_PLATFORM_WIN32_KHR)
elseif(UNIX AND NOT APPLE)
    target_compile_definitions(ForceEngine PRIVATE VK_USE_PLATFORM_XCB_KHR)
endif()

# Compiler warnings
if(MSVC)
    target_compile_options(ForceEngine PRIVATE /W4 /WX-)
else()
    target_compile_options(ForceEngine PRIVATE -Wall -Wextra -Wpedantic)
endif()
