#pragma once
#include "Force/Core/Base.h"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>

namespace Force
{
    static constexpr u32 DDGI_IRRADIANCE_TEXELS = 8;
    static constexpr u32 DDGI_DEPTH_TEXELS      = 16;

    struct DDGIDesc
    {
        glm::ivec3 GridDims       = { 8, 4, 8 };
        f32        ProbeSpacing   = 4.0f;
        glm::vec3  GridOrigin     = { -16.0f, 0.0f, -16.0f };
        u32        RaysPerProbe   = 64;
        u32        ProbesPerFrame = 32;
        f32        Hysteresis     = 0.97f;
    };

    struct alignas(16) DDGIProbeUBO
    {
        glm::vec4  GridOriginSpacing;  // xyz=origin, w=spacing
        glm::ivec4 GridDims;           // xyz=dims, w=raysPerProbe
        glm::vec4  Params;             // x=hysteresis, y=probesPerFrame, z=totalProbes, w=frameIdx
    };

    class DDGISystem
    {
    public:
        static DDGISystem& Get();

        void Init(const DDGIDesc& desc);
        void Shutdown();

        void Update(VkCommandBuffer cmd,
                    VkImageView gbufferAlbedo,
                    VkImageView gbufferNormal,
                    VkImageView gbufferDepth,
                    const glm::mat4& invViewProj,
                    u32 frameIndex);

        VkImageView GetIrradianceAtlas() const { return m_IrradianceView; }
        VkImageView GetDepthAtlas()      const { return m_DepthView; }
        VkSampler   GetProbeSampler()    const { return m_Sampler; }
        VkBuffer    GetProbeUBO()        const { return m_ProbeUBO; }
        bool        IsInitialized()      const { return m_Initialized; }

    private:
        void CreateAtlases();
        void CreatePipeline();
        void CreateDescriptors();

    private:
        DDGIDesc m_Desc;
        bool     m_Initialized = false;
        u32      m_TotalProbes  = 0;

        VkImage       m_IrradianceImage = VK_NULL_HANDLE;
        VmaAllocation m_IrradianceAlloc = VK_NULL_HANDLE;
        VkImageView   m_IrradianceView  = VK_NULL_HANDLE;

        VkImage       m_DepthImage  = VK_NULL_HANDLE;
        VmaAllocation m_DepthAlloc  = VK_NULL_HANDLE;
        VkImageView   m_DepthView   = VK_NULL_HANDLE;

        VkSampler m_Sampler = VK_NULL_HANDLE;

        VkPipeline       m_Pipeline = VK_NULL_HANDLE;
        VkPipelineLayout m_Layout   = VK_NULL_HANDLE;

        VkDescriptorPool      m_Pool = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_DSL  = VK_NULL_HANDLE;
        VkDescriptorSet       m_DS   = VK_NULL_HANDLE;

        VkBuffer      m_ProbeUBO      = VK_NULL_HANDLE;
        VmaAllocation m_ProbeUBOAlloc = VK_NULL_HANDLE;
        void*         m_ProbeUBOMapped = nullptr;
    };
}
