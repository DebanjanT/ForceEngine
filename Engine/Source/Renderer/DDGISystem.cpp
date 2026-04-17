#include "Force/Renderer/DDGISystem.h"
#include "Force/Renderer/VulkanDevice.h"
#include "Force/Core/Logger.h"
#include <vk_mem_alloc.h>
#include <fstream>
#include <vector>
#include <cstring>

namespace Force
{
    static VkShaderModule LoadSPIRV(const char* path)
    {
        std::ifstream f(path, std::ios::binary | std::ios::ate);
        if (!f) { FORCE_CORE_ERROR("DDGISystem: cannot open shader {}", path); return VK_NULL_HANDLE; }
        size_t size = (size_t)f.tellg(); f.seekg(0);
        std::vector<u32> code(size / 4);
        f.read(reinterpret_cast<char*>(code.data()), size);
        VkShaderModuleCreateInfo ci{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
        ci.codeSize = size; ci.pCode = code.data();
        VkShaderModule mod = VK_NULL_HANDLE;
        vkCreateShaderModule(VulkanDevice::Get().GetDevice(), &ci, nullptr, &mod);
        return mod;
    }

    DDGISystem& DDGISystem::Get()
    {
        static DDGISystem instance;
        return instance;
    }

    void DDGISystem::Init(const DDGIDesc& desc)
    {
        if (m_Initialized) Shutdown();
        m_Desc = desc;
        m_TotalProbes = desc.GridDims.x * desc.GridDims.y * desc.GridDims.z;
        CreateAtlases();
        CreateDescriptors();
        CreatePipeline();
        m_Initialized = true;
        FORCE_CORE_INFO("DDGISystem: {}x{}x{} probes ({} total)", desc.GridDims.x, desc.GridDims.y, desc.GridDims.z, m_TotalProbes);
    }

    void DDGISystem::Shutdown()
    {
        if (!m_Initialized) return;
        VkDevice     device    = VulkanDevice::Get().GetDevice();
        VmaAllocator allocator = VulkanDevice::Get().GetAllocator();
        VulkanDevice::Get().WaitIdle();

        if (m_ProbeUBOMapped) vmaUnmapMemory(allocator, m_ProbeUBOAlloc);
        if (m_ProbeUBO)       vmaDestroyBuffer(allocator, m_ProbeUBO, m_ProbeUBOAlloc);
        if (m_Sampler)        vkDestroySampler(device, m_Sampler, nullptr);
        if (m_Pipeline)       vkDestroyPipeline(device, m_Pipeline, nullptr);
        if (m_Layout)         vkDestroyPipelineLayout(device, m_Layout, nullptr);
        if (m_DSL)            vkDestroyDescriptorSetLayout(device, m_DSL, nullptr);
        if (m_Pool)           vkDestroyDescriptorPool(device, m_Pool, nullptr);

        if (m_IrradianceView)  vkDestroyImageView(device, m_IrradianceView, nullptr);
        if (m_IrradianceImage) vmaDestroyImage(allocator, m_IrradianceImage, m_IrradianceAlloc);
        if (m_DepthView)       vkDestroyImageView(device, m_DepthView, nullptr);
        if (m_DepthImage)      vmaDestroyImage(allocator, m_DepthImage, m_DepthAlloc);

        m_Initialized = false;
    }

    void DDGISystem::CreateAtlases()
    {
        VkDevice     device    = VulkanDevice::Get().GetDevice();
        VmaAllocator allocator = VulkanDevice::Get().GetAllocator();

        // Atlas layout: all probes in a 1D strip arranged as (totalProbes * texelsPerProbe, 1) 2D texture
        // Lay out as (probesX * probesZ * irradianceTexels, probesY * irradianceTexels)
        u32 atlasW_irr = (u32)(m_Desc.GridDims.x * m_Desc.GridDims.z) * DDGI_IRRADIANCE_TEXELS;
        u32 atlasH_irr = (u32)m_Desc.GridDims.y * DDGI_IRRADIANCE_TEXELS;
        u32 atlasW_dep = (u32)(m_Desc.GridDims.x * m_Desc.GridDims.z) * DDGI_DEPTH_TEXELS;
        u32 atlasH_dep = (u32)m_Desc.GridDims.y * DDGI_DEPTH_TEXELS;

        auto createAtlas = [&](u32 w, u32 h, VkFormat fmt, VkImage& img, VmaAllocation& alloc, VkImageView& view)
        {
            VkImageCreateInfo imageCI{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
            imageCI.imageType     = VK_IMAGE_TYPE_2D;
            imageCI.format        = fmt;
            imageCI.extent        = { w, h, 1 };
            imageCI.mipLevels     = 1;
            imageCI.arrayLayers   = 1;
            imageCI.samples       = VK_SAMPLE_COUNT_1_BIT;
            imageCI.tiling        = VK_IMAGE_TILING_OPTIMAL;
            imageCI.usage         = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            VmaAllocationCreateInfo aCI{}; aCI.usage = VMA_MEMORY_USAGE_GPU_ONLY;
            vmaCreateImage(allocator, &imageCI, &aCI, &img, &alloc, nullptr);

            VkImageViewCreateInfo viewCI{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
            viewCI.image    = img; viewCI.viewType = VK_IMAGE_VIEW_TYPE_2D; viewCI.format = fmt;
            viewCI.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
            vkCreateImageView(device, &viewCI, nullptr, &view);
        };

        createAtlas(atlasW_irr, atlasH_irr, VK_FORMAT_R16G16B16A16_SFLOAT,
                    m_IrradianceImage, m_IrradianceAlloc, m_IrradianceView);
        createAtlas(atlasW_dep, atlasH_dep, VK_FORMAT_R16G16_SFLOAT,
                    m_DepthImage, m_DepthAlloc, m_DepthView);

        VkSamplerCreateInfo sCI{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
        sCI.magFilter  = VK_FILTER_LINEAR; sCI.minFilter = VK_FILTER_LINEAR;
        sCI.addressModeU = sCI.addressModeV = sCI.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        vkCreateSampler(device, &sCI, nullptr, &m_Sampler);

        VkBufferCreateInfo bCI{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        bCI.size = sizeof(DDGIProbeUBO); bCI.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        VmaAllocationCreateInfo uboCI{}; uboCI.usage = VMA_MEMORY_USAGE_CPU_TO_GPU; uboCI.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
        VmaAllocationInfo uboInfo{};
        vmaCreateBuffer(allocator, &bCI, &uboCI, &m_ProbeUBO, &m_ProbeUBOAlloc, &uboInfo);
        m_ProbeUBOMapped = uboInfo.pMappedData;
    }

    void DDGISystem::CreateDescriptors()
    {
        VkDevice device = VulkanDevice::Get().GetDevice();

        VkDescriptorPoolSize sizes[] = {
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,   2 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,   3 },
            { VK_DESCRIPTOR_TYPE_SAMPLER,         1 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,  1 },
        };
        VkDescriptorPoolCreateInfo poolCI{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
        poolCI.maxSets = 1; poolCI.poolSizeCount = 4; poolCI.pPoolSizes = sizes;
        vkCreateDescriptorPool(device, &poolCI, nullptr, &m_Pool);

        VkDescriptorSetLayoutBinding bindings[7]{};
        bindings[0] = { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,  1, VK_SHADER_STAGE_COMPUTE_BIT }; // probe UBO
        bindings[1] = { 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,   1, VK_SHADER_STAGE_COMPUTE_BIT }; // irradiance atlas (write)
        bindings[2] = { 2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,   1, VK_SHADER_STAGE_COMPUTE_BIT }; // depth atlas (write)
        bindings[3] = { 3, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,   1, VK_SHADER_STAGE_COMPUTE_BIT }; // gbuffer albedo
        bindings[4] = { 4, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,   1, VK_SHADER_STAGE_COMPUTE_BIT }; // gbuffer normal
        bindings[5] = { 5, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,   1, VK_SHADER_STAGE_COMPUTE_BIT }; // gbuffer depth
        bindings[6] = { 6, VK_DESCRIPTOR_TYPE_SAMPLER,         1, VK_SHADER_STAGE_COMPUTE_BIT }; // sampler

        VkDescriptorSetLayoutCreateInfo dslCI{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
        dslCI.bindingCount = 7; dslCI.pBindings = bindings;
        vkCreateDescriptorSetLayout(device, &dslCI, nullptr, &m_DSL);

        VkDescriptorSetAllocateInfo allocI{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
        allocI.descriptorPool = m_Pool; allocI.descriptorSetCount = 1; allocI.pSetLayouts = &m_DSL;
        vkAllocateDescriptorSets(device, &allocI, &m_DS);

        // Write UBO + atlas images (GBuffer images written in Update())
        VkDescriptorBufferInfo uboInfo{ m_ProbeUBO, 0, sizeof(DDGIProbeUBO) };
        VkDescriptorImageInfo  irrInfo{ VK_NULL_HANDLE, m_IrradianceView, VK_IMAGE_LAYOUT_GENERAL };
        VkDescriptorImageInfo  depInfo{ VK_NULL_HANDLE, m_DepthView,      VK_IMAGE_LAYOUT_GENERAL };
        VkDescriptorImageInfo  sampInfo{ m_Sampler, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_UNDEFINED };

        VkWriteDescriptorSet writes[4]{};
        for (auto& w : writes) w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].dstSet = m_DS; writes[0].dstBinding = 0; writes[0].descriptorCount = 1;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; writes[0].pBufferInfo = &uboInfo;
        writes[1].dstSet = m_DS; writes[1].dstBinding = 1; writes[1].descriptorCount = 1;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;  writes[1].pImageInfo = &irrInfo;
        writes[2].dstSet = m_DS; writes[2].dstBinding = 2; writes[2].descriptorCount = 1;
        writes[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;  writes[2].pImageInfo = &depInfo;
        writes[3].dstSet = m_DS; writes[3].dstBinding = 6; writes[3].descriptorCount = 1;
        writes[3].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;        writes[3].pImageInfo = &sampInfo;
        vkUpdateDescriptorSets(device, 4, writes, 0, nullptr);
    }

    void DDGISystem::CreatePipeline()
    {
        VkDevice device = VulkanDevice::Get().GetDevice();
        VkShaderModule mod = LoadSPIRV("Assets/Shaders/Compiled/DDGIUpdate.comp.spv");

        VkPipelineLayoutCreateInfo layoutCI{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
        layoutCI.setLayoutCount = 1; layoutCI.pSetLayouts = &m_DSL;
        vkCreatePipelineLayout(device, &layoutCI, nullptr, &m_Layout);

        VkPipelineShaderStageCreateInfo stage{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
        stage.stage = VK_SHADER_STAGE_COMPUTE_BIT; stage.module = mod; stage.pName = "main";
        VkComputePipelineCreateInfo ci{ VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
        ci.stage = stage; ci.layout = m_Layout;
        vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &ci, nullptr, &m_Pipeline);
        vkDestroyShaderModule(device, mod, nullptr);
    }

    void DDGISystem::Update(VkCommandBuffer cmd,
                             VkImageView gbufferAlbedo,
                             VkImageView gbufferNormal,
                             VkImageView gbufferDepth,
                             const glm::mat4& invViewProj,
                             u32 frameIndex)
    {
        if (!m_Initialized) return;

        // Update probe UBO
        if (m_ProbeUBOMapped)
        {
            DDGIProbeUBO ubo{};
            ubo.GridOriginSpacing = glm::vec4(m_Desc.GridOrigin, m_Desc.ProbeSpacing);
            ubo.GridDims          = glm::ivec4(m_Desc.GridDims, (i32)m_Desc.RaysPerProbe);
            ubo.Params            = glm::vec4(m_Desc.Hysteresis, (f32)m_Desc.ProbesPerFrame, (f32)m_TotalProbes, (f32)frameIndex);
            memcpy(m_ProbeUBOMapped, &ubo, sizeof(ubo));
        }

        // Update GBuffer image descriptors
        VkDescriptorImageInfo albedoInfo{ VK_NULL_HANDLE, gbufferAlbedo, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
        VkDescriptorImageInfo normalInfo{ VK_NULL_HANDLE, gbufferNormal, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
        VkDescriptorImageInfo depthInfo { VK_NULL_HANDLE, gbufferDepth,  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

        VkWriteDescriptorSet writes[3]{};
        for (auto& w : writes) { w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET; w.dstSet = m_DS; w.descriptorCount = 1; w.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE; }
        writes[0].dstBinding = 3; writes[0].pImageInfo = &albedoInfo;
        writes[1].dstBinding = 4; writes[1].pImageInfo = &normalInfo;
        writes[2].dstBinding = 5; writes[2].pImageInfo = &depthInfo;
        vkUpdateDescriptorSets(VulkanDevice::Get().GetDevice(), 3, writes, 0, nullptr);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_Pipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_Layout, 0, 1, &m_DS, 0, nullptr);

        u32 groups = (m_Desc.ProbesPerFrame + 63) / 64;
        vkCmdDispatch(cmd, groups, 1, 1);

        VkMemoryBarrier barrier{ VK_STRUCTURE_TYPE_MEMORY_BARRIER };
        barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(cmd,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0, 1, &barrier, 0, nullptr, 0, nullptr);
    }
}
