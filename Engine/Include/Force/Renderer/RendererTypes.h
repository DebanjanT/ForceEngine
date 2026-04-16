#pragma once

#include "Force/Core/Types.h"
#include <vulkan/vulkan.h>

namespace Force
{
    // Forward declarations
    class VulkanContext;
    class VulkanDevice;
    class VulkanSwapchain;
    class VulkanPipeline;
    class VulkanBuffer;
    class VulkanCommandBuffer;
    
    // Renderer configuration
    struct RendererConfig
    {
        u32 MaxFramesInFlight = 2;
        bool EnableValidation = true;
        bool VSync = true;
    };
    
    // Buffer types
    enum class BufferType : u8
    {
        Vertex,
        Index,
        Uniform,
        Storage,
        Staging
    };
    
    // Texture formats
    enum class TextureFormat : u8
    {
        Unknown,
        R8,
        RG8,
        RGB8,
        RGBA8,
        R16F,
        RG16F,
        RGB16F,
        RGBA16F,
        R32F,
        RG32F,
        RGB32F,
        RGBA32F,
        Depth24Stencil8,
        Depth32F
    };
    
    // Render pass types
    enum class RenderPassType : u8
    {
        Graphics,
        Compute,
        Transfer
    };
    
    // Cull modes
    enum class CullMode : u8
    {
        None,
        Front,
        Back,
        FrontAndBack
    };
    
    // Polygon modes
    enum class PolygonMode : u8
    {
        Fill,
        Line,
        Point
    };
    
    // Blend modes
    enum class BlendMode : u8
    {
        None,
        Alpha,
        Additive,
        Multiply
    };
    
    // Pipeline state
    struct PipelineState
    {
        CullMode Culling = CullMode::Back;
        PolygonMode Polygon = PolygonMode::Fill;
        BlendMode Blend = BlendMode::None;
        bool DepthTest = true;
        bool DepthWrite = true;
        bool Wireframe = false;
    };
}
