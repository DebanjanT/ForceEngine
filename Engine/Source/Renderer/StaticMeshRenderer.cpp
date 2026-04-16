#include "Force/Renderer/StaticMeshRenderer.h"
#include "Force/Renderer/Renderer.h"
#include "Force/Core/Logger.h"
#include <algorithm>

namespace Force
{
    StaticMeshRenderer& StaticMeshRenderer::Get()
    {
        static StaticMeshRenderer instance;
        return instance;
    }

    void StaticMeshRenderer::Init(u32 maxDraws, u32 maxOcclusionQueries)
    {
        if (m_Initialized) return;

        m_MaxDraws   = maxDraws;
        m_MaxQueries = maxOcclusionQueries;

        // Allocate indirect draw buffer (host-visible for CPU fill)
        m_IndirectBuffer.Create(
            sizeof(VkDrawIndexedIndirectCommand) * m_MaxDraws,
            VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU);

        // Occlusion query pool
        VkQueryPoolCreateInfo queryInfo{};
        queryInfo.sType      = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        queryInfo.queryType  = VK_QUERY_TYPE_OCCLUSION;
        queryInfo.queryCount = m_MaxQueries;

        VkResult result = vkCreateQueryPool(Renderer::GetDevice().GetDevice(),
                                             &queryInfo, nullptr, &m_QueryPool);
        FORCE_CORE_ASSERT(result == VK_SUCCESS, "Failed to create occlusion query pool!");

        m_QueryResults.resize(m_MaxQueries, 0);
        m_Initialized = true;

        FORCE_CORE_INFO("StaticMeshRenderer initialized ({} draws, {} queries)",
                        m_MaxDraws, m_MaxQueries);
    }

    void StaticMeshRenderer::Shutdown()
    {
        if (!m_Initialized) return;

        if (m_QueryPool != VK_NULL_HANDLE)
        {
            vkDestroyQueryPool(Renderer::GetDevice().GetDevice(), m_QueryPool, nullptr);
            m_QueryPool = VK_NULL_HANDLE;
        }
        m_IndirectBuffer.Destroy();
        m_DrawList.clear();
        m_Initialized = false;
    }

    void StaticMeshRenderer::Reset()
    {
        m_DrawList.clear();
        m_IndirectCommandCount = 0;
        m_QueryCount = 0;
    }

    void StaticMeshRenderer::Submit(const Ref<Mesh>& mesh, const Ref<Material>& material,
                                     const glm::mat4& transform, u32 subMeshIndex)
    {
        if (!mesh) return;
        MeshDrawCommand cmd;
        cmd.MeshPtr      = mesh;
        cmd.MaterialPtr  = material;
        cmd.Transform    = transform;
        cmd.SubMeshIndex = subMeshIndex;
        cmd.ObjectId     = static_cast<u32>(m_DrawList.size());
        m_DrawList.push_back(cmd);
    }

    void StaticMeshRenderer::Flush(VkCommandBuffer cmd,
                                    const glm::mat4& viewProj,
                                    const glm::vec3& cameraPos)
    {
        if (m_DrawList.empty()) return;

        // Fetch occlusion results from last frame before we overwrite query pool
        FetchOcclusionResults();

        Frustum frustum = Frustum::FromViewProjection(viewProj);

        // Frustum cull
        std::vector<MeshDrawCommand> visible;
        visible.reserve(m_DrawList.size());
        for (const auto& dc : m_DrawList)
        {
            AABB worldAABB;
            worldAABB.Min = dc.MeshPtr->GetBoundingBoxMin();
            worldAABB.Max = dc.MeshPtr->GetBoundingBoxMax();
            worldAABB     = worldAABB.Transform(dc.Transform);

            if (frustum.TestAABB(worldAABB))
                visible.push_back(dc);
        }

        if (visible.empty()) return;

        // Build indirect buffer
        BuildIndirectBuffer(cmd, visible);

        // Reset query pool for this frame
        if (m_QueryPool != VK_NULL_HANDLE && !visible.empty())
        {
            vkCmdResetQueryPool(cmd, m_QueryPool, 0,
                                std::min(static_cast<u32>(visible.size()), m_MaxQueries));
        }

        // Issue draws grouped by mesh+material (naive batching)
        Ref<Mesh>     lastMesh     = nullptr;
        Ref<Material> lastMaterial = nullptr;
        u32           batchStart   = 0;
        u32           batchCount   = 0;

        auto flushBatch = [&]()
        {
            if (batchCount == 0 || !lastMesh) return;

            lastMesh->Bind(cmd);
            if (lastMaterial)
                lastMaterial->Bind(cmd, VK_NULL_HANDLE); // layout bound by shader

            VkDeviceSize offset = batchStart * sizeof(VkDrawIndexedIndirectCommand);
            vkCmdDrawIndexedIndirect(cmd, m_IndirectBuffer.GetBuffer(),
                                     offset, batchCount,
                                     sizeof(VkDrawIndexedIndirectCommand));
            batchStart += batchCount;
            batchCount  = 0;
        };

        for (u32 i = 0; i < static_cast<u32>(visible.size()); ++i)
        {
            const auto& dc = visible[i];
            if (dc.MeshPtr != lastMesh || dc.MaterialPtr != lastMaterial)
            {
                flushBatch();
                lastMesh     = dc.MeshPtr;
                lastMaterial = dc.MaterialPtr;
            }
            ++batchCount;
        }
        flushBatch();
    }

    void StaticMeshRenderer::BuildIndirectBuffer(VkCommandBuffer /*cmd*/,
                                                  const std::vector<MeshDrawCommand>& visible)
    {
        auto* commands = static_cast<VkDrawIndexedIndirectCommand*>(
            m_IndirectBuffer.Map());
        if (!commands) return;

        u32 count = std::min(static_cast<u32>(visible.size()), m_MaxDraws);
        for (u32 i = 0; i < count; ++i)
        {
            const auto& dc  = visible[i];
            const auto& sub = dc.MeshPtr->GetSubMeshes();
            bool valid = !sub.empty() && dc.SubMeshIndex < sub.size();

            commands[i].indexCount    = valid ? sub[dc.SubMeshIndex].IndexCount
                                               : dc.MeshPtr->GetIndexCount();
            commands[i].instanceCount = 1;
            commands[i].firstIndex    = valid ? sub[dc.SubMeshIndex].BaseIndex  : 0;
            commands[i].vertexOffset  = valid ? sub[dc.SubMeshIndex].BaseVertex : 0;
            commands[i].firstInstance = i; // used to index transform in push-constant
        }

        m_IndirectBuffer.Unmap();
        m_IndirectCommandCount = count;
    }

    void StaticMeshRenderer::FetchOcclusionResults()
    {
        if (m_QueryPool == VK_NULL_HANDLE || m_QueryCount == 0) return;

        vkGetQueryPoolResults(
            Renderer::GetDevice().GetDevice(),
            m_QueryPool, 0, m_QueryCount,
            m_QueryCount * sizeof(u64),
            m_QueryResults.data(),
            sizeof(u64),
            VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
    }

    bool StaticMeshRenderer::IsVisible(u32 objectId) const
    {
        if (objectId >= static_cast<u32>(m_QueryResults.size())) return true;
        return m_QueryResults[objectId] > 0;
    }
}
