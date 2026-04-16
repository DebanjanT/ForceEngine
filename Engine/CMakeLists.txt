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
    Source/Renderer/Mesh.cpp
    Source/Renderer/Shader.cpp
    Source/Renderer/Texture.cpp
    Source/Renderer/RenderGraph.cpp
    Source/Renderer/Material.cpp
    Source/Renderer/DebugRenderer.cpp
    
    # Asset
    Source/Asset/AssetRegistry.cpp
    Source/Asset/AssetImporter.cpp
    Source/Asset/MeshAsset.cpp
    
    # Scene
    Source/Scene/SceneSerializer.cpp
    
    # Physics
    Source/Physics/PhysicsEngine.cpp
    Source/Physics/PhysicsBody.cpp
    Source/Physics/CharacterController.cpp
    Source/Physics/PhysicsDebugRenderer.cpp
    Source/Physics/DestructionSystem.cpp

    # Particle System & VFX
    Source/Renderer/ParticleSystem.cpp
    Source/Renderer/DecalSystem.cpp
    Source/VFX/VFXGraph.cpp

    # Animation
    Source/Animation/Socket.cpp
    Source/Animation/Skeleton.cpp
    Source/Animation/AnimationClip.cpp
    Source/Animation/AnimationStateMachine.cpp
    Source/Animation/AnimNotify.cpp

    # Audio
    Source/Audio/AudioSource.cpp
    Source/Audio/AudioEngine.cpp

    # Renderer (Static Mesh)
    Source/Renderer/FrustumCuller.cpp
    Source/Renderer/StaticMeshRenderer.cpp

    # Editor
    Source/Editor/SceneHierarchyPanel.cpp
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
    Include/Force/Renderer/Mesh.h
    Include/Force/Renderer/Shader.h
    Include/Force/Renderer/Texture.h
    Include/Force/Renderer/RenderGraph.h
    Include/Force/Renderer/Material.h
    Include/Force/Renderer/DebugRenderer.h
    
    # Math
    Include/Force/Math/Math.h
    
    # Asset
    Include/Force/Asset/AssetTypes.h
    Include/Force/Asset/Asset.h
    Include/Force/Asset/AssetRegistry.h
    Include/Force/Asset/AssetImporter.h
    Include/Force/Asset/TextureAsset.h
    Include/Force/Asset/MeshAsset.h
    
    # Scene
    Include/Force/Scene/SceneSerializer.h
    Include/Force/Scene/Components.h
    
    # Physics
    Include/Force/Physics/PhysicsEngine.h
    Include/Force/Physics/PhysicsBody.h
    Include/Force/Physics/CharacterController.h
    Include/Force/Physics/PhysicsDebugRenderer.h
    Include/Force/Physics/DestructionSystem.h

    # Particle System & VFX
    Include/Force/Renderer/ParticleSystem.h
    Include/Force/Renderer/DecalSystem.h
    Include/Force/VFX/VFXGraph.h

    # Animation
    Include/Force/Animation/Socket.h
    Include/Force/Animation/Skeleton.h
    Include/Force/Animation/AnimationClip.h
    Include/Force/Animation/AnimationStateMachine.h
    Include/Force/Animation/AnimNotify.h

    # Audio
    Include/Force/Audio/AudioSource.h
    Include/Force/Audio/AudioEngine.h

    # Renderer (Static Mesh)
    Include/Force/Renderer/FrustumCuller.h
    Include/Force/Renderer/StaticMeshRenderer.h

    # Editor
    Include/Force/Editor/SceneHierarchyPanel.h
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
        stb
        assimp::assimp
        nlohmann_json::nlohmann_json
        imgui
        OpenAL
        Jolt
)

target_compile_definitions(ForceEngine PUBLIC
    FORCE_AUDIO_OPENAL
    FORCE_PHYSICS_JOLT
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
