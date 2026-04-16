#pragma once

#include "Force/Core/Base.h"
#include "Force/Renderer/Mesh.h"
#include "Force/Renderer/Material.h"
#include "Force/Renderer/VulkanBuffer.h"
#include "Force/Renderer/FrustumCuller.h"
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <vector>

namespace Force
{
    struct MeshDrawCommand
    {
        Ref<Mesh>     MeshPtr;
        Ref<Material> MaterialPtr;
        glm::mat4     Transform    = glm::mat4(1.0f);
        u32           SubMeshIndex = 0;
        u32           ObjectId     = 0;   // for occlusion query correlation
    };

    class StaticMeshRenderer
    {
    public:
        static StaticMeshRenderer& Get();

        void Init(u32 maxDraws = 4096, u32 maxOcclusionQueries = 1024);
        void Shutdown();

        // Call once per frame before submitting
        void Reset();

        // Queue a mesh for this frame
        void Submit(const Ref<Mesh>& mesh, const Ref<Material>& material,
                    const glm::mat4& transform, u32 subMeshIndex = 0);

        // Cull + LOD-select + build indirect buffer + issue draws
        void Flush(VkCommandBuffer cmd, const glm::mat4& viewProj,
                   const glm::vec3& cameraPos);

        // Read-back occlusion result from previous frame (0 = culled)
        bool IsVisible(u32 objectId) const;

    private:
        void BuildIndirectBuffer(VkCommandBuffer cmd,
                                 const std::vector<MeshDrawCommand>& visible);
        void BeginOcclusionQueries(VkCommandBuffer cmd);
        void EndOcclusionQueries(VkCommandBuffer cmd);
        void FetchOcclusionResults();

    private:
        std::vector<MeshDrawCommand> m_DrawList;

        // GPU indirect draw buffer  (VkDrawIndexedIndirectCommand * maxDraws)
        VulkanBuffer m_IndirectBuffer;
        u32          m_IndirectCommandCount = 0;
        u32          m_MaxDraws             = 4096;

        // Occlusion query pool
        VkQueryPool        m_QueryPool    = VK_NULL_HANDLE;
        u32                m_MaxQueries   = 1024;
        u32                m_QueryCount   = 0;
        std::vector<u64>   m_QueryResults;

        bool m_Initialized = false;
    };
}
