#include "Force/Terrain/TerrainPainter.h"
#include "Force/Renderer/VulkanDevice.h"
#include "Force/Core/Logger.h"
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>
#include <cstring>
#include <cmath>
#include <algorithm>

namespace Force
{
    void TerrainPainter::Init(u32 blendMapWidth, u32 blendMapHeight)
    {
        m_W = blendMapWidth;
        m_H = blendMapHeight;
        m_Map0.assign(m_W * m_H * 4, 0);
        m_Map1.assign(m_W * m_H * 4, 0);
        // Default: layer 0 fully painted
        for (u32 i = 0; i < m_W * m_H; i++)
            m_Map0[i * 4] = 255;
        CreateImages();
        UploadBlendMaps();
    }

    void TerrainPainter::Shutdown()
    {
        VkDevice     device    = VulkanDevice::Get().GetDevice();
        VmaAllocator allocator = VulkanDevice::Get().GetAllocator();
        VulkanDevice::Get().WaitIdle();
        if (m_Sampler)    vkDestroySampler(device, m_Sampler, nullptr);
        if (m_Map0View)   vkDestroyImageView(device, m_Map0View, nullptr);
        if (m_Map1View)   vkDestroyImageView(device, m_Map1View, nullptr);
        if (m_Map0Image)  vmaDestroyImage(allocator, m_Map0Image, m_Map0Alloc);
        if (m_Map1Image)  vmaDestroyImage(allocator, m_Map1Image, m_Map1Alloc);
    }

    void TerrainPainter::CreateImages()
    {
        VkDevice     device    = VulkanDevice::Get().GetDevice();
        VmaAllocator allocator = VulkanDevice::Get().GetAllocator();

        auto createImg = [&](VkImage& img, VmaAllocation& alloc, VkImageView& view)
        {
            VkImageCreateInfo imageCI{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
            imageCI.imageType     = VK_IMAGE_TYPE_2D;
            imageCI.format        = VK_FORMAT_R8G8B8A8_UNORM;
            imageCI.extent        = { m_W, m_H, 1 };
            imageCI.mipLevels     = 1; imageCI.arrayLayers = 1;
            imageCI.samples       = VK_SAMPLE_COUNT_1_BIT;
            imageCI.tiling        = VK_IMAGE_TILING_OPTIMAL;
            imageCI.usage         = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            VmaAllocationCreateInfo aCI{}; aCI.usage = VMA_MEMORY_USAGE_GPU_ONLY;
            vmaCreateImage(allocator, &imageCI, &aCI, &img, &alloc, nullptr);
            VkImageViewCreateInfo viewCI{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
            viewCI.image = img; viewCI.viewType = VK_IMAGE_VIEW_TYPE_2D; viewCI.format = VK_FORMAT_R8G8B8A8_UNORM;
            viewCI.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
            vkCreateImageView(device, &viewCI, nullptr, &view);
        };
        createImg(m_Map0Image, m_Map0Alloc, m_Map0View);
        createImg(m_Map1Image, m_Map1Alloc, m_Map1View);

        VkSamplerCreateInfo sCI{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
        sCI.magFilter = VK_FILTER_LINEAR; sCI.minFilter = VK_FILTER_LINEAR;
        sCI.addressModeU = sCI.addressModeV = sCI.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        vkCreateSampler(device, &sCI, nullptr, &m_Sampler);
    }

    static void UploadToImage(VkImage img, const std::vector<u8>& data, u32 w, u32 h)
    {
        VkDeviceSize size = w * h * 4;
        VkBuffer staging; VmaAllocation stagingAlloc;
        VkBufferCreateInfo bCI{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        bCI.size = size; bCI.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        VmaAllocationCreateInfo aCI{}; aCI.usage = VMA_MEMORY_USAGE_CPU_ONLY; aCI.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
        VmaAllocationInfo info{};
        vmaCreateBuffer(VulkanDevice::Get().GetAllocator(), &bCI, &aCI, &staging, &stagingAlloc, &info);
        memcpy(info.pMappedData, data.data(), size);

        VkCommandBufferAllocateInfo cbAI{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        cbAI.commandPool = VulkanDevice::Get().GetCommandPool();
        cbAI.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; cbAI.commandBufferCount = 1;
        VkCommandBuffer cb;
        vkAllocateCommandBuffers(VulkanDevice::Get().GetDevice(), &cbAI, &cb);
        VkCommandBufferBeginInfo bi{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cb, &bi);

        VkImageMemoryBarrier bar{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
        bar.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED; bar.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        bar.image = img; bar.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        bar.srcAccessMask = 0; bar.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &bar);

        VkBufferImageCopy region{};
        region.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
        region.imageExtent = { w, h, 1 };
        vkCmdCopyBufferToImage(cb, staging, img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        bar.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL; bar.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        bar.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; bar.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &bar);

        vkEndCommandBuffer(cb);
        VkSubmitInfo si{ VK_STRUCTURE_TYPE_SUBMIT_INFO }; si.commandBufferCount = 1; si.pCommandBuffers = &cb;
        vkQueueSubmit(VulkanDevice::Get().GetGraphicsQueue(), 1, &si, VK_NULL_HANDLE);
        vkQueueWaitIdle(VulkanDevice::Get().GetGraphicsQueue());
        vkFreeCommandBuffers(VulkanDevice::Get().GetDevice(), VulkanDevice::Get().GetCommandPool(), 1, &cb);
        vmaDestroyBuffer(VulkanDevice::Get().GetAllocator(), staging, stagingAlloc);
    }

    void TerrainPainter::UploadBlendMaps()
    {
        UploadToImage(m_Map0Image, m_Map0, m_W, m_H);
        UploadToImage(m_Map1Image, m_Map1, m_W, m_H);
    }

    u32 TerrainPainter::AddLayer(const TerrainLayer& layer)
    {
        if (m_Layers.size() >= MAX_TERRAIN_LAYERS)
        { FORCE_CORE_WARN("TerrainPainter: max {} layers reached", MAX_TERRAIN_LAYERS); return (u32)m_Layers.size() - 1; }
        m_Layers.push_back(layer);
        return (u32)m_Layers.size() - 1;
    }

    void TerrainPainter::SetLayer(u32 index, const TerrainLayer& layer)
    {
        if (index < m_Layers.size()) m_Layers[index] = layer;
    }

    void TerrainPainter::NormalizeWeights(u32 x, u32 z)
    {
        u32 idx = z * m_W + x;
        float sum = 0;
        u8* m0 = m_Map0.data() + idx * 4;
        u8* m1 = m_Map1.data() + idx * 4;
        for (int i = 0; i < 4; i++) { sum += m0[i]; sum += m1[i]; }
        if (sum < 1.0f) { m0[0] = 255; return; } // fallback to layer 0
        float scale = 255.0f / sum;
        for (int i = 0; i < 4; i++) { m0[i] = (u8)(m0[i] * scale); m1[i] = (u8)(m1[i] * scale); }
    }

    void TerrainPainter::PaintBrush(const PaintBrushDesc& desc, f32 terrainWorldSize)
    {
        if (desc.LayerIndex >= m_Layers.size()) return;

        f32 texelSize = terrainWorldSize / (f32)m_W;
        i32 radiusTex = (i32)std::ceil(desc.Radius / texelSize) + 1;
        i32 cx = (i32)(desc.WorldPosXZ.x / terrainWorldSize * m_W);
        i32 cz = (i32)(desc.WorldPosXZ.y / terrainWorldSize * m_H);

        for (i32 dz = -radiusTex; dz <= radiusTex; dz++)
        for (i32 dx = -radiusTex; dx <= radiusTex; dx++)
        {
            i32 x = cx + dx, z = cz + dz;
            if (x < 0 || z < 0 || x >= (i32)m_W || z >= (i32)m_H) continue;

            f32 wx = x * texelSize, wz = z * texelSize;
            f32 dist = glm::length(glm::vec2(wx - desc.WorldPosXZ.x, wz - desc.WorldPosXZ.y));
            if (dist > desc.Radius) continue;

            f32 t      = 1.0f - glm::clamp(dist / desc.Radius, 0.0f, 1.0f);
            f32 weight = t * t * (3.0f - 2.0f * t);
            f32 delta  = desc.Strength * weight * 255.0f;

            u32 idx     = (u32)(z * m_W + x);
            u32 layer   = desc.LayerIndex;
            u8* map     = layer < 4 ? m_Map0.data() : m_Map1.data();
            u32 channel = layer % 4;

            map[idx * 4 + channel] = (u8)glm::clamp((f32)map[idx * 4 + channel] + delta, 0.0f, 255.0f);
            NormalizeWeights((u32)x, (u32)z);
        }
    }
}
