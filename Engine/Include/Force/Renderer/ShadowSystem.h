#pragma once
#include "Force/Core/Base.h"
#include "Force/Renderer/LightTypes.h"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>
#include <array>

namespace Force
{
    static constexpr u32 MAX_SHADOW_CASCADES = 4;

    struct alignas(16) CascadeData
    {
        glm::mat4 ViewProj[MAX_SHADOW_CASCADES];
        glm::vec4 SplitDepths; // x,y,z,w = far plane of each cascade (view space)
    };

    struct ShadowMapDesc
    {
        u32 Resolution   = 2048;
        u32 CascadeCount = 4;
        f32 LambdaSplit  = 0.75f; // 0=linear 1=logarithmic
        f32 NearPlane    = 0.1f;
        f32 FarPlane     = 500.0f;
    };

    class ShadowSystem
    {
    public:
        static ShadowSystem& Get();

        void Init(const ShadowMapDesc& desc);
        void Shutdown();

        void UpdateCascades(const DirectionalLight& sun,
                            const glm::mat4& camView, const glm::mat4& camProj,
                            f32 camNear, f32 camFar);

        void BeginCascade(VkCommandBuffer cmd, u32 cascadeIndex);
        void EndCascade(VkCommandBuffer cmd);

        VkImageView        GetCascadeView(u32 index) const { return m_Views[index]; }
        VkSampler          GetShadowSampler()         const { return m_Sampler; }
        VkBuffer           GetCascadeUBO()            const { return m_UBO; }
        const CascadeData& GetCascadeData()           const { return m_CascadeData; }
        bool               IsInitialized()            const { return m_Initialized; }

    private:
        void CreateDepthResources();
        void CreateRenderPass();
        void CreateFramebuffers();
        void CreateUBO();
        void CreateSampler();

    private:
        ShadowMapDesc m_Desc;
        bool          m_Initialized = false;

        std::array<VkImage,        MAX_SHADOW_CASCADES> m_Images      = {};
        std::array<VmaAllocation,  MAX_SHADOW_CASCADES> m_Allocs      = {};
        std::array<VkImageView,    MAX_SHADOW_CASCADES> m_Views       = {};
        std::array<VkFramebuffer,  MAX_SHADOW_CASCADES> m_Framebuffers = {};

        VkRenderPass  m_RenderPass = VK_NULL_HANDLE;
        VkSampler     m_Sampler    = VK_NULL_HANDLE;

        VkBuffer      m_UBO        = VK_NULL_HANDLE;
        VmaAllocation m_UBOAlloc   = VK_NULL_HANDLE;
        void*         m_UBOMapped  = nullptr;

        CascadeData m_CascadeData;
    };
}
