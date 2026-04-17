#pragma once
#include "Force/Core/Base.h"
#include "Force/Renderer/LightTypes.h"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>
#include <vector>

namespace Force
{
    static constexpr u32 CLUSTER_X             = 16;
    static constexpr u32 CLUSTER_Y             = 9;
    static constexpr u32 CLUSTER_Z             = 24;
    static constexpr u32 CLUSTER_COUNT         = CLUSTER_X * CLUSTER_Y * CLUSTER_Z;
    static constexpr u32 MAX_LIGHTS_PER_CLUSTER = 64;
    static constexpr u32 MAX_POINT_LIGHTS       = 256;
    static constexpr u32 MAX_SPOT_LIGHTS        = 128;

    struct ClusteringDesc
    {
        f32 NearPlane = 0.1f;
        f32 FarPlane  = 500.0f;
        u32 Width     = 1920;
        u32 Height    = 1080;
    };

    struct alignas(16) ClusterUBO
    {
        glm::mat4  InvProj;
        glm::vec4  ScreenNearFar; // xy=screen size, zw=near/far
        glm::uvec4 ClusterDims;   // xyz=cluster counts
    };

    // Per-cluster: offset into global light index list + count
    struct alignas(8) ClusterLightGrid
    {
        u32 Offset;
        u32 PointCount;
        u32 SpotCount;
        u32 _pad;
    };

    class ClusteredLighting
    {
    public:
        static ClusteredLighting& Get();

        void Init(const ClusteringDesc& desc);
        void Shutdown();

        void Reset();
        void SubmitPointLight(const PointLight& light);
        void SubmitSpotLight(const SpotLight& light);
        void SetDirectionalLight(const DirectionalLight& light);

        // Compute: build cluster AABBs (once per proj change) + cull lights
        void Flush(VkCommandBuffer cmd, const glm::mat4& proj, bool rebuildAABBs = false);

        // Bind light buffers to descriptor set for shading pass
        void BindDescriptors(VkCommandBuffer cmd, VkPipelineLayout layout, u32 set);

        VkDescriptorSetLayout GetDSL() const    { return m_DSL; }
        VkBuffer GetDirLightBuffer()  const     { return m_DirLightBuf; }
        bool     IsInitialized()      const     { return m_Initialized; }

    private:
        void CreateBuffers();
        void CreatePipelines();
        void CreateDescriptors();

    private:
        ClusteringDesc m_Desc;
        bool           m_Initialized = false;

        std::vector<PointLight> m_PointLights;
        std::vector<SpotLight>  m_SpotLights;
        DirectionalLight        m_DirLight;

        VkBuffer m_ClusterAABBBuf  = VK_NULL_HANDLE;  VmaAllocation m_ClusterAABBAlloc  = VK_NULL_HANDLE;
        VkBuffer m_LightGridBuf    = VK_NULL_HANDLE;  VmaAllocation m_LightGridAlloc    = VK_NULL_HANDLE;
        VkBuffer m_LightIndexBuf   = VK_NULL_HANDLE;  VmaAllocation m_LightIndexAlloc   = VK_NULL_HANDLE;
        VkBuffer m_PointLightBuf   = VK_NULL_HANDLE;  VmaAllocation m_PointLightAlloc   = VK_NULL_HANDLE;
        VkBuffer m_SpotLightBuf    = VK_NULL_HANDLE;  VmaAllocation m_SpotLightAlloc    = VK_NULL_HANDLE;
        VkBuffer m_DirLightBuf     = VK_NULL_HANDLE;  VmaAllocation m_DirLightAlloc     = VK_NULL_HANDLE;
        VkBuffer m_ClusterUBO      = VK_NULL_HANDLE;  VmaAllocation m_ClusterUBOAlloc   = VK_NULL_HANDLE;
        void*    m_ClusterUBOMapped = nullptr;

        VkPipeline       m_BuildPipeline = VK_NULL_HANDLE;
        VkPipelineLayout m_BuildLayout   = VK_NULL_HANDLE;
        VkPipeline       m_CullPipeline  = VK_NULL_HANDLE;
        VkPipelineLayout m_CullLayout    = VK_NULL_HANDLE;

        VkDescriptorPool      m_Pool    = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_CompDSL = VK_NULL_HANDLE;
        VkDescriptorSet       m_CompDS  = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_DSL     = VK_NULL_HANDLE;
        VkDescriptorSet       m_DS      = VK_NULL_HANDLE;
    };
}
