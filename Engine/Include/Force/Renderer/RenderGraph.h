#pragma once

#include "Force/Core/Base.h"
#include "Force/Renderer/Texture.h"
#include <vulkan/vulkan.h>
#include <functional>
#include <string>
#include <vector>
#include <unordered_map>

namespace Force
{
    // Typed handle for render graph resources
    using RGHandle = u32;
    constexpr RGHandle RG_INVALID_HANDLE = ~0u;

    // Description for a transient texture resource
    struct RGTextureDesc
    {
        u32      Width      = 0;  // 0 = use swapchain size
        u32      Height     = 0;
        VkFormat Format     = VK_FORMAT_R8G8B8A8_UNORM;
        VkImageUsageFlags ExtraUsage = 0; // additional usage flags beyond what RG deduces
    };

    // Internal resource tracking
    struct RGResource
    {
        std::string       Name;
        RGTextureDesc     Desc;
        Ref<Texture2D>    Texture;      // backing texture (created after Compile)
        VkImageLayout     CurrentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        bool              IsImported    = false;
        bool              IsWritten     = false;  // has any pass written to this?
    };

    // Render pass callback signature
    using RGPassExecuteFn = std::function<void(VkCommandBuffer cmd)>;

    // Forward declaration
    class RenderGraph;

    // ----------------------------------------------------------------------------
    // PassBuilder — fluent API for declaring pass I/O
    // ----------------------------------------------------------------------------
    class PassBuilder
    {
    public:
        PassBuilder(RenderGraph& graph, u32 passIndex);

        // Declare that this pass writes to a color attachment
        PassBuilder& AddColorOutput(const std::string& name, const RGTextureDesc& desc);
        PassBuilder& AddColorOutput(RGHandle handle);

        // Declare that this pass writes to a depth attachment
        PassBuilder& SetDepthOutput(const std::string& name, const RGTextureDesc& desc);
        PassBuilder& SetDepthOutput(RGHandle handle);

        // Declare that this pass reads a texture (shader sample)
        PassBuilder& AddTextureInput(RGHandle handle);

        // Set the execution callback
        void Execute(RGPassExecuteFn fn);

    private:
        RenderGraph& m_Graph;
        u32          m_PassIndex;
    };

    // ----------------------------------------------------------------------------
    // Internal pass representation
    // ----------------------------------------------------------------------------
    struct RGPass
    {
        std::string              Name;
        std::vector<RGHandle>    ColorOutputs;
        RGHandle                 DepthOutput = RG_INVALID_HANDLE;
        std::vector<RGHandle>    TextureInputs;
        RGPassExecuteFn          ExecuteFn;
    };

    // ----------------------------------------------------------------------------
    // RenderGraph — frame graph abstraction
    // ----------------------------------------------------------------------------
    class RenderGraph
    {
    public:
        RenderGraph() = default;
        ~RenderGraph() = default;

        // Declare a new transient texture (returns handle)
        RGHandle DeclareTexture(const std::string& name, const RGTextureDesc& desc);

        // Import an external texture (e.g., swapchain image, loaded texture)
        RGHandle ImportTexture(const std::string& name, const Ref<Texture2D>& texture);

        // Add a render pass (returns PassBuilder for fluent configuration)
        PassBuilder AddPass(const std::string& name);

        // Compile the graph — creates backing resources, computes execution order
        void Compile(u32 framebufferWidth, u32 framebufferHeight);

        // Execute all passes in order, recording into the provided command buffer
        void Execute(VkCommandBuffer cmd);

        // Handle window resize — recreates framebuffer-sized resources
        void OnResize(u32 width, u32 height);

        // Retrieve a texture by handle (valid after Compile)
        Ref<Texture2D> GetTexture(RGHandle handle) const;

        // Retrieve a texture by name (valid after Compile)
        Ref<Texture2D> GetTexture(const std::string& name) const;

        // Reset for next frame (clears passes, keeps resources if size unchanged)
        void Reset();

        // Full cleanup
        void Destroy();

    private:
        friend class PassBuilder;

        void CreateResources();
        void InsertBarriers(VkCommandBuffer cmd, const RGPass& pass);

        RGHandle GetOrCreateHandle(const std::string& name, const RGTextureDesc& desc);

    private:
        std::vector<RGResource>                    m_Resources;
        std::unordered_map<std::string, RGHandle>  m_NameToHandle;
        std::vector<RGPass>                        m_Passes;
        std::vector<u32>                           m_ExecutionOrder; // pass indices in execution order

        u32  m_FramebufferWidth  = 0;
        u32  m_FramebufferHeight = 0;
        bool m_Compiled          = false;
    };
}
