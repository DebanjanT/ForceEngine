#pragma once

#include "Force/Core/Base.h"
#include "Force/Renderer/Texture.h"
#include "Force/Renderer/VulkanBuffer.h"
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>

namespace Force
{
    // -------------------------------------------------------------------------
    struct DecalDesc
    {
        glm::vec3      Position      = glm::vec3(0.0f);
        glm::quat      Rotation      = glm::quat(1, 0, 0, 0);
        glm::vec3      HalfExtents   = { 0.5f, 0.5f, 0.5f };  // projection box

        Ref<Texture2D> AlbedoTexture;
        Ref<Texture2D> NormalTexture;   // optional

        glm::vec4      Color          = glm::vec4(1.0f);
        f32            Opacity        = 1.0f;
        f32            FadeStartDist  = 8.0f;
        f32            FadeEndDist    = 12.0f;
        f32            Lifetime       = -1.0f;   // <0 = permanent
        f32            FadeOutTime    = 1.0f;    // seconds to fade when expired
        u32            SortPriority   = 0;
    };

    // -------------------------------------------------------------------------
    // GPU-side decal instance (std140 packed)
    struct alignas(16) DecalGPU
    {
        glm::mat4  InverseWorld;     // world → decal local space
        glm::vec4  Color;
        glm::vec2  AtlasTile;        // for texture atlas support
        f32        Opacity;
        f32        _pad;
    };

    // -------------------------------------------------------------------------
    class DecalSystem
    {
    public:
        static DecalSystem& Get();

        void Init(u32 maxDecals = 512);
        void Shutdown();

        u32  AddDecal(const DecalDesc& desc);
        void RemoveDecal(u32 id);
        void Clear();

        // Tick — advance ages, fade out, reclaim expired decals
        void Update(f32 dt);

        // Deferred decal pass: projects decals onto GBuffer depth
        // Must be called after GBuffer fill, depth buffer already resolved
        void Render(VkCommandBuffer cmd,
                    VkImageView     gbufferDepth,
                    VkImageView     gbufferAlbedo,
                    VkImageView     gbufferNormal,
                    const glm::mat4& invViewProj,
                    const glm::vec3& cameraPos);

        u32 GetActiveCount() const;

    private:
        void CreatePipeline();
        void UploadDecalBuffer();

    private:
        struct DecalInstance
        {
            DecalDesc Desc;
            u32       Id         = UINT32_MAX;
            f32       Age        = 0.0f;
            f32       FadeAlpha  = 1.0f;
            bool      FadingOut  = false;
        };

        std::vector<DecalInstance> m_Decals;
        u32                        m_MaxDecals  = 512;
        u32                        m_NextId     = 0;

        VulkanBuffer               m_DecalBuffer;   // DecalGPU * maxDecals

        VkPipeline                 m_Pipeline       = VK_NULL_HANDLE;
        VkPipelineLayout           m_PipelineLayout = VK_NULL_HANDLE;
        VkDescriptorSetLayout      m_DSL            = VK_NULL_HANDLE;
        VkDescriptorSet            m_DS             = VK_NULL_HANDLE;

        bool m_Initialized = false;
    };
}
