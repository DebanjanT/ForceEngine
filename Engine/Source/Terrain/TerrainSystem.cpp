#include "Force/Terrain/TerrainSystem.h"
#include "Force/Renderer/VulkanDevice.h"
#include "Force/Core/Logger.h"
#include <vk_mem_alloc.h>
#include <glm/gtc/matrix_transform.hpp>
#include <cstring>
#include <cmath>
#include <algorithm>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace Force
{
    TerrainSystem& TerrainSystem::Get()
    {
        static TerrainSystem instance;
        return instance;
    }

    void TerrainSystem::Init(const TerrainDesc& desc)
    {
        if (m_Initialized) Shutdown();
        m_Desc = desc;
        LoadHeightmap();
        CreateHeightmapImage();
        GenerateChunks();
        m_Initialized = true;
        FORCE_CORE_INFO("TerrainSystem: {}x{} chunks, heightmap {}x{}",
            desc.ChunkCountX, desc.ChunkCountZ, m_HmapW, m_HmapH);
    }

    void TerrainSystem::Shutdown()
    {
        if (!m_Initialized) return;
        VkDevice     device    = VulkanDevice::Get().GetDevice();
        VmaAllocator allocator = VulkanDevice::Get().GetAllocator();
        VulkanDevice::Get().WaitIdle();

        for (auto& chunk : m_Chunks)
        {
            chunk.VertexBuf.Destroy();
        }
        for (auto& ib : m_SharedIndexBufs) ib.Destroy();

        if (m_HmapSampler) vkDestroySampler(device, m_HmapSampler, nullptr);
        if (m_HmapView)    vkDestroyImageView(device, m_HmapView, nullptr);
        if (m_HmapImage)   vmaDestroyImage(allocator, m_HmapImage, m_HmapAlloc);

        m_Chunks.clear();
        m_Heightmap.clear();
        m_Initialized = false;
    }

    void TerrainSystem::LoadHeightmap()
    {
        if (!m_Desc.HeightmapPath.empty())
        {
            int w, h, c;
            // Try 16-bit PNG first
            u16* data16 = stbi_load_16(m_Desc.HeightmapPath.c_str(), &w, &h, &c, 1);
            if (data16)
            {
                m_HmapW = (u32)w; m_HmapH = (u32)h;
                m_Heightmap.resize(m_HmapW * m_HmapH);
                for (u32 i = 0; i < m_HmapW * m_HmapH; i++)
                    m_Heightmap[i] = (data16[i] / 65535.0f) * m_Desc.MaxHeight;
                stbi_image_free(data16);
                return;
            }
            // Fall back to 8-bit
            u8* data8 = stbi_load(m_Desc.HeightmapPath.c_str(), &w, &h, &c, 1);
            if (data8)
            {
                m_HmapW = (u32)w; m_HmapH = (u32)h;
                m_Heightmap.resize(m_HmapW * m_HmapH);
                for (u32 i = 0; i < m_HmapW * m_HmapH; i++)
                    m_Heightmap[i] = (data8[i] / 255.0f) * m_Desc.MaxHeight;
                stbi_image_free(data8);
                return;
            }
            FORCE_CORE_WARN("TerrainSystem: could not load heightmap '{}', using flat terrain", m_Desc.HeightmapPath);
        }
        // Flat fallback
        m_HmapW = m_HmapH = 512;
        m_Heightmap.assign(m_HmapW * m_HmapH, 0.0f);
    }

    void TerrainSystem::CreateHeightmapImage()
    {
        VkDevice     device    = VulkanDevice::Get().GetDevice();
        VmaAllocator allocator = VulkanDevice::Get().GetAllocator();

        VkImageCreateInfo imageCI{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
        imageCI.imageType     = VK_IMAGE_TYPE_2D;
        imageCI.format        = VK_FORMAT_R32_SFLOAT;
        imageCI.extent        = { m_HmapW, m_HmapH, 1 };
        imageCI.mipLevels     = 1; imageCI.arrayLayers = 1;
        imageCI.samples       = VK_SAMPLE_COUNT_1_BIT;
        imageCI.tiling        = VK_IMAGE_TILING_OPTIMAL;
        imageCI.usage         = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VmaAllocationCreateInfo aCI{}; aCI.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        vmaCreateImage(allocator, &imageCI, &aCI, &m_HmapImage, &m_HmapAlloc, nullptr);

        VkImageViewCreateInfo viewCI{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        viewCI.image    = m_HmapImage; viewCI.viewType = VK_IMAGE_VIEW_TYPE_2D; viewCI.format = VK_FORMAT_R32_SFLOAT;
        viewCI.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        vkCreateImageView(device, &viewCI, nullptr, &m_HmapView);

        VkSamplerCreateInfo sCI{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
        sCI.magFilter = VK_FILTER_LINEAR; sCI.minFilter = VK_FILTER_LINEAR;
        sCI.addressModeU = sCI.addressModeV = sCI.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        vkCreateSampler(device, &sCI, nullptr, &m_HmapSampler);

        UploadHeightmap();
    }

    void TerrainSystem::UploadHeightmap()
    {
        // Create staging buffer and copy
        VkDeviceSize dataSize = m_HmapW * m_HmapH * sizeof(f32);
        VkBuffer      staging; VmaAllocation stagingAlloc;
        VkBufferCreateInfo bCI{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        bCI.size = dataSize; bCI.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        VmaAllocationCreateInfo sACI{}; sACI.usage = VMA_MEMORY_USAGE_CPU_ONLY; sACI.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
        VmaAllocationInfo sInfo{};
        vmaCreateBuffer(VulkanDevice::Get().GetAllocator(), &bCI, &sACI, &staging, &stagingAlloc, &sInfo);
        memcpy(sInfo.pMappedData, m_Heightmap.data(), dataSize);

        // Single-time command buffer
        VkCommandBufferAllocateInfo cbAI{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        cbAI.commandPool        = VulkanDevice::Get().GetCommandPool();
        cbAI.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cbAI.commandBufferCount = 1;
        VkCommandBuffer cb;
        vkAllocateCommandBuffers(VulkanDevice::Get().GetDevice(), &cbAI, &cb);

        VkCommandBufferBeginInfo beginI{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        beginI.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cb, &beginI);

        // Transition to transfer dst
        VkImageMemoryBarrier barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
        barrier.oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.image               = m_HmapImage;
        barrier.subresourceRange    = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        barrier.srcAccessMask       = 0;
        barrier.dstAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
        vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        VkBufferImageCopy region{};
        region.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
        region.imageExtent      = { m_HmapW, m_HmapH, 1 };
        vkCmdCopyBufferToImage(cb, staging, m_HmapImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        vkEndCommandBuffer(cb);
        VkSubmitInfo submitI{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submitI.commandBufferCount = 1; submitI.pCommandBuffers = &cb;
        vkQueueSubmit(VulkanDevice::Get().GetGraphicsQueue(), 1, &submitI, VK_NULL_HANDLE);
        vkQueueWaitIdle(VulkanDevice::Get().GetGraphicsQueue());
        vkFreeCommandBuffers(VulkanDevice::Get().GetDevice(), VulkanDevice::Get().GetCommandPool(), 1, &cb);
        vmaDestroyBuffer(VulkanDevice::Get().GetAllocator(), staging, stagingAlloc);
    }

    f32 TerrainSystem::SampleHeight(f32 wx, f32 wz) const
    {
        f32 u = (wx / m_Desc.WorldSize) * (m_HmapW - 1);
        f32 v = (wz / m_Desc.WorldSize) * (m_HmapH - 1);
        u = glm::clamp(u, 0.0f, (f32)(m_HmapW - 1));
        v = glm::clamp(v, 0.0f, (f32)(m_HmapH - 1));
        u32 x0 = (u32)u, z0 = (u32)v;
        u32 x1 = glm::min(x0 + 1, m_HmapW - 1);
        u32 z1 = glm::min(z0 + 1, m_HmapH - 1);
        f32 fx = u - x0, fz = v - z0;
        f32 h00 = m_Heightmap[z0 * m_HmapW + x0];
        f32 h10 = m_Heightmap[z0 * m_HmapW + x1];
        f32 h01 = m_Heightmap[z1 * m_HmapW + x0];
        f32 h11 = m_Heightmap[z1 * m_HmapW + x1];
        return glm::mix(glm::mix(h00, h10, fx), glm::mix(h01, h11, fx), fz);
    }

    glm::vec3 TerrainSystem::ComputeNormal(f32 wx, f32 wz) const
    {
        f32 step = m_Desc.WorldSize / (f32)(m_HmapW - 1);
        f32 hL = SampleHeight(wx - step, wz);
        f32 hR = SampleHeight(wx + step, wz);
        f32 hD = SampleHeight(wx, wz - step);
        f32 hU = SampleHeight(wx, wz + step);
        return glm::normalize(glm::vec3(hL - hR, 2.0f * step, hD - hU));
    }

    void TerrainSystem::GenerateChunks()
    {
        // Build shared LOD index buffers
        for (u32 lod = 0; lod < TERRAIN_MAX_LOD; lod++)
        {
            u32 step = 1u << lod;
            u32 vertsPerEdge = (TERRAIN_CHUNK_VERTS - 1) / step + 1;
            u32 quads = vertsPerEdge - 1;
            std::vector<u32> indices;
            indices.reserve(quads * quads * 6);
            for (u32 z = 0; z < quads; z++)
                for (u32 x = 0; x < quads; x++)
                {
                    u32 i0 = z * vertsPerEdge + x;
                    u32 i1 = i0 + 1;
                    u32 i2 = (z + 1) * vertsPerEdge + x;
                    u32 i3 = i2 + 1;
                    indices.insert(indices.end(), { i0, i2, i1, i1, i2, i3 });
                }
            m_SharedIndexCounts[lod] = (u32)indices.size();
            m_SharedIndexBufs[lod].Create(indices.size() * sizeof(u32),
                VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                VMA_MEMORY_USAGE_GPU_ONLY);
            m_SharedIndexBufs[lod].SetData(indices.data(), indices.size() * sizeof(u32));
        }

        f32 chunkSize = m_Desc.WorldSize / m_Desc.ChunkCountX;
        m_Chunks.resize(m_Desc.ChunkCountX * m_Desc.ChunkCountZ);
        for (u32 z = 0; z < m_Desc.ChunkCountZ; z++)
            for (u32 x = 0; x < m_Desc.ChunkCountX; x++)
            {
                auto& chunk   = m_Chunks[z * m_Desc.ChunkCountX + x];
                chunk.GridPos  = { (i32)x, (i32)z };
                chunk.ChunkSize = chunkSize;
                GenerateChunkMesh(chunk);
                chunk.Loaded = true;
            }
    }

    void TerrainSystem::GenerateChunkMesh(TerrainChunk& chunk)
    {
        f32 chunkSize = chunk.ChunkSize;
        f32 originX   = chunk.GridPos.x * chunkSize;
        f32 originZ   = chunk.GridPos.y * chunkSize;
        f32 step      = chunkSize / (TERRAIN_CHUNK_VERTS - 1);

        std::vector<TerrainVertex> verts;
        verts.reserve(TERRAIN_CHUNK_VERTS * TERRAIN_CHUNK_VERTS);

        for (u32 z = 0; z < TERRAIN_CHUNK_VERTS; z++)
            for (u32 x = 0; x < TERRAIN_CHUNK_VERTS; x++)
            {
                f32 wx = originX + x * step;
                f32 wz = originZ + z * step;
                TerrainVertex v;
                v.Position = { wx, SampleHeight(wx, wz), wz };
                v.Normal   = ComputeNormal(wx, wz);
                v.UV       = { wx / m_Desc.WorldSize, wz / m_Desc.WorldSize };
                verts.push_back(v);
            }

        chunk.VertexBuf.Create(verts.size() * sizeof(TerrainVertex),
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY);
        chunk.VertexBuf.SetData(verts.data(), verts.size() * sizeof(TerrainVertex));
    }

    u32 TerrainSystem::SelectLOD(const TerrainChunk& chunk, const glm::vec3& camPos) const
    {
        glm::vec3 center = {
            (chunk.GridPos.x + 0.5f) * chunk.ChunkSize,
            0.0f,
            (chunk.GridPos.y + 0.5f) * chunk.ChunkSize
        };
        f32 dist = glm::length(glm::vec2(camPos.x - center.x, camPos.z - center.z));
        for (u32 lod = 0; lod < TERRAIN_MAX_LOD; lod++)
        {
            f32 threshold = m_Desc.LODDistanceBase * (1u << lod);
            if (dist < threshold) return lod;
        }
        return TERRAIN_MAX_LOD - 1;
    }

    void TerrainSystem::Update(const glm::vec3& cameraPos)
    {
        for (auto& chunk : m_Chunks)
        {
            if (!chunk.Loaded) continue;
            chunk.CurrentLOD = SelectLOD(chunk, cameraPos);

            // Simple visibility: always visible (frustum culling done by caller if needed)
            chunk.Visible = true;
        }
    }

    void TerrainSystem::Draw(VkCommandBuffer cmd, VkPipelineLayout layout)
    {
        for (auto& chunk : m_Chunks)
        {
            if (!chunk.Loaded || !chunk.Visible) continue;

            VkBuffer vb     = chunk.VertexBuf.GetBuffer();
            VkDeviceSize off = 0;
            vkCmdBindVertexBuffers(cmd, 0, 1, &vb, &off);

            u32 lod = chunk.CurrentLOD;
            vkCmdBindIndexBuffer(cmd, m_SharedIndexBufs[lod].GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

            // Push chunk origin as push constant
            glm::vec4 origin(chunk.GridPos.x * chunk.ChunkSize, 0, chunk.GridPos.y * chunk.ChunkSize, chunk.ChunkSize);
            vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::vec4), &origin);

            vkCmdDrawIndexed(cmd, m_SharedIndexCounts[lod], 1, 0, 0, 0);
        }
    }
}
