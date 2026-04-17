#include "Force/Renderer/ClusteredLighting.h"
#include "Force/Renderer/VulkanDevice.h"
#include "Force/Core/Logger.h"
#include <vk_mem_alloc.h>
#include <fstream>
#include <vector>
#include <cstring>
#include <algorithm>

namespace Force
{
    static VkShaderModule LoadSPIRV(const char* path)
    {
        std::ifstream f(path, std::ios::binary | std::ios::ate);
        if (!f) { FORCE_CORE_ERROR("ClusteredLighting: cannot open shader {}", path); return VK_NULL_HANDLE; }
        size_t size = (size_t)f.tellg(); f.seekg(0);
        std::vector<u32> code(size / 4);
        f.read(reinterpret_cast<char*>(code.data()), size);
        VkShaderModuleCreateInfo ci{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
        ci.codeSize = size; ci.pCode = code.data();
        VkShaderModule mod = VK_NULL_HANDLE;
        vkCreateShaderModule(VulkanDevice::Get().GetDevice(), &ci, nullptr, &mod);
        return mod;
    }

    static void CreateMappedBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                                   VkBuffer& buf, VmaAllocation& alloc, void*& mapped)
    {
        VkBufferCreateInfo bCI{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        bCI.size  = size; bCI.usage = usage;
        VmaAllocationCreateInfo aCI{};
        aCI.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        aCI.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
        VmaAllocationInfo info{};
        vmaCreateBuffer(VulkanDevice::Get().GetAllocator(), &bCI, &aCI, &buf, &alloc, &info);
        mapped = info.pMappedData;
    }

    static void CreateGPUBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                                VkBuffer& buf, VmaAllocation& alloc)
    {
        VkBufferCreateInfo bCI{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        bCI.size = size; bCI.usage = usage;
        VmaAllocationCreateInfo aCI{};
        aCI.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        vmaCreateBuffer(VulkanDevice::Get().GetAllocator(), &bCI, &aCI, &buf, &alloc, nullptr);
    }

    ClusteredLighting& ClusteredLighting::Get()
    {
        static ClusteredLighting instance;
        return instance;
    }

    void ClusteredLighting::Init(const ClusteringDesc& desc)
    {
        if (m_Initialized) Shutdown();
        m_Desc = desc;
        CreateBuffers();
        CreateDescriptors();
        CreatePipelines();
        m_Initialized = true;
        FORCE_CORE_INFO("ClusteredLighting: {}x{}x{} clusters, max {} point + {} spot lights",
            CLUSTER_X, CLUSTER_Y, CLUSTER_Z, MAX_POINT_LIGHTS, MAX_SPOT_LIGHTS);
    }

    void ClusteredLighting::Shutdown()
    {
        if (!m_Initialized) return;
        VkDevice     device    = VulkanDevice::Get().GetDevice();
        VmaAllocator allocator = VulkanDevice::Get().GetAllocator();
        VulkanDevice::Get().WaitIdle();

        auto destroyBuf = [&](VkBuffer& b, VmaAllocation& a){ if (b) { vmaDestroyBuffer(allocator, b, a); b = VK_NULL_HANDLE; } };
        destroyBuf(m_ClusterAABBBuf, m_ClusterAABBAlloc);
        destroyBuf(m_LightGridBuf,   m_LightGridAlloc);
        destroyBuf(m_LightIndexBuf,  m_LightIndexAlloc);
        destroyBuf(m_PointLightBuf,  m_PointLightAlloc);
        destroyBuf(m_SpotLightBuf,   m_SpotLightAlloc);
        destroyBuf(m_DirLightBuf,    m_DirLightAlloc);
        destroyBuf(m_ClusterUBO,     m_ClusterUBOAlloc);

        if (m_BuildPipeline)  vkDestroyPipeline(device, m_BuildPipeline, nullptr);
        if (m_BuildLayout)    vkDestroyPipelineLayout(device, m_BuildLayout, nullptr);
        if (m_CullPipeline)   vkDestroyPipeline(device, m_CullPipeline, nullptr);
        if (m_CullLayout)     vkDestroyPipelineLayout(device, m_CullLayout, nullptr);
        if (m_CompDSL)        vkDestroyDescriptorSetLayout(device, m_CompDSL, nullptr);
        if (m_DSL)            vkDestroyDescriptorSetLayout(device, m_DSL, nullptr);
        if (m_Pool)           vkDestroyDescriptorPool(device, m_Pool, nullptr);
        m_Initialized = false;
    }

    void ClusteredLighting::CreateBuffers()
    {
        // AABB buffer: 6 floats per cluster (min xyz, max xyz) = vec4 min + vec4 max
        CreateGPUBuffer(CLUSTER_COUNT * sizeof(glm::vec4) * 2, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, m_ClusterAABBBuf, m_ClusterAABBAlloc);
        // Light grid: per-cluster offset+counts
        CreateGPUBuffer(CLUSTER_COUNT * sizeof(ClusterLightGrid), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, m_LightGridBuf, m_LightGridAlloc);
        // Global light index list
        CreateGPUBuffer(CLUSTER_COUNT * MAX_LIGHTS_PER_CLUSTER * sizeof(u32) * 2, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, m_LightIndexBuf, m_LightIndexAlloc);

        void* dummy = nullptr;
        CreateMappedBuffer(sizeof(PointLight) * MAX_POINT_LIGHTS,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, m_PointLightBuf, m_PointLightAlloc, dummy);
        CreateMappedBuffer(sizeof(SpotLight) * MAX_SPOT_LIGHTS,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, m_SpotLightBuf, m_SpotLightAlloc, dummy);
        CreateMappedBuffer(sizeof(DirectionalLight),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, m_DirLightBuf, m_DirLightAlloc, dummy);
        CreateMappedBuffer(sizeof(ClusterUBO),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, m_ClusterUBO, m_ClusterUBOAlloc, m_ClusterUBOMapped);
    }

    void ClusteredLighting::CreateDescriptors()
    {
        VkDevice device = VulkanDevice::Get().GetDevice();

        VkDescriptorPoolSize sizes[] = {
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,  8 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,  4 },
        };
        VkDescriptorPoolCreateInfo poolCI{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
        poolCI.maxSets = 2; poolCI.poolSizeCount = 2; poolCI.pPoolSizes = sizes;
        vkCreateDescriptorPool(device, &poolCI, nullptr, &m_Pool);

        // Compute DSL: UBO + 5 SSBOs
        VkDescriptorSetLayoutBinding compBindings[6]{};
        compBindings[0] = { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,  1, VK_SHADER_STAGE_COMPUTE_BIT };
        compBindings[1] = { 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,  1, VK_SHADER_STAGE_COMPUTE_BIT }; // AABB
        compBindings[2] = { 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,  1, VK_SHADER_STAGE_COMPUTE_BIT }; // point lights
        compBindings[3] = { 3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,  1, VK_SHADER_STAGE_COMPUTE_BIT }; // spot lights
        compBindings[4] = { 4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,  1, VK_SHADER_STAGE_COMPUTE_BIT }; // light grid
        compBindings[5] = { 5, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,  1, VK_SHADER_STAGE_COMPUTE_BIT }; // light index
        VkDescriptorSetLayoutCreateInfo compDSLCI{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
        compDSLCI.bindingCount = 6; compDSLCI.pBindings = compBindings;
        vkCreateDescriptorSetLayout(device, &compDSLCI, nullptr, &m_CompDSL);

        // Shading DSL: point + spot + dir + grid + index
        VkDescriptorSetLayoutBinding shadBindings[5]{};
        shadBindings[0] = { 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,  1, VK_SHADER_STAGE_FRAGMENT_BIT }; // point
        shadBindings[1] = { 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,  1, VK_SHADER_STAGE_FRAGMENT_BIT }; // spot
        shadBindings[2] = { 2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,  1, VK_SHADER_STAGE_FRAGMENT_BIT }; // dir
        shadBindings[3] = { 3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,  1, VK_SHADER_STAGE_FRAGMENT_BIT }; // grid
        shadBindings[4] = { 4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,  1, VK_SHADER_STAGE_FRAGMENT_BIT }; // index
        VkDescriptorSetLayoutCreateInfo shadDSLCI{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
        shadDSLCI.bindingCount = 5; shadDSLCI.pBindings = shadBindings;
        vkCreateDescriptorSetLayout(device, &shadDSLCI, nullptr, &m_DSL);

        VkDescriptorSetLayout layouts[2] = { m_CompDSL, m_DSL };
        VkDescriptorSetAllocateInfo allocI{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
        allocI.descriptorPool = m_Pool; allocI.descriptorSetCount = 2; allocI.pSetLayouts = layouts;
        VkDescriptorSet sets[2]{};
        vkAllocateDescriptorSets(device, &allocI, sets);
        m_CompDS = sets[0]; m_DS = sets[1];

        // Write compute DS
        VkDescriptorBufferInfo cUBOInfo  { m_ClusterUBO,    0, sizeof(ClusterUBO) };
        VkDescriptorBufferInfo aabbInfo  { m_ClusterAABBBuf, 0, VK_WHOLE_SIZE };
        VkDescriptorBufferInfo ptInfo    { m_PointLightBuf,  0, VK_WHOLE_SIZE };
        VkDescriptorBufferInfo spInfo    { m_SpotLightBuf,   0, VK_WHOLE_SIZE };
        VkDescriptorBufferInfo gridInfo  { m_LightGridBuf,   0, VK_WHOLE_SIZE };
        VkDescriptorBufferInfo idxInfo   { m_LightIndexBuf,  0, VK_WHOLE_SIZE };

        VkWriteDescriptorSet cWrites[6]{};
        for (u32 i = 0; i < 6; i++) { cWrites[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET; cWrites[i].dstSet = m_CompDS; cWrites[i].dstBinding = i; cWrites[i].descriptorCount = 1; }
        cWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;  cWrites[0].pBufferInfo = &cUBOInfo;
        cWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;  cWrites[1].pBufferInfo = &aabbInfo;
        cWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;  cWrites[2].pBufferInfo = &ptInfo;
        cWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;  cWrites[3].pBufferInfo = &spInfo;
        cWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;  cWrites[4].pBufferInfo = &gridInfo;
        cWrites[5].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;  cWrites[5].pBufferInfo = &idxInfo;
        vkUpdateDescriptorSets(device, 6, cWrites, 0, nullptr);

        // Write shading DS
        VkDescriptorBufferInfo dirInfo{ m_DirLightBuf, 0, sizeof(DirectionalLight) };
        VkWriteDescriptorSet sWrites[5]{};
        for (u32 i = 0; i < 5; i++) { sWrites[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET; sWrites[i].dstSet = m_DS; sWrites[i].dstBinding = i; sWrites[i].descriptorCount = 1; }
        sWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;  sWrites[0].pBufferInfo = &ptInfo;
        sWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;  sWrites[1].pBufferInfo = &spInfo;
        sWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;  sWrites[2].pBufferInfo = &dirInfo;
        sWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;  sWrites[3].pBufferInfo = &gridInfo;
        sWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;  sWrites[4].pBufferInfo = &idxInfo;
        vkUpdateDescriptorSets(device, 5, sWrites, 0, nullptr);
    }

    void ClusteredLighting::CreatePipelines()
    {
        VkDevice device = VulkanDevice::Get().GetDevice();

        VkPipelineLayoutCreateInfo layoutCI{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
        layoutCI.setLayoutCount = 1; layoutCI.pSetLayouts = &m_CompDSL;
        vkCreatePipelineLayout(device, &layoutCI, nullptr, &m_BuildLayout);
        vkCreatePipelineLayout(device, &layoutCI, nullptr, &m_CullLayout);

        auto makeCompute = [&](const char* spvPath, VkPipelineLayout layout, VkPipeline& pipeline)
        {
            VkShaderModule mod = LoadSPIRV(spvPath);
            VkPipelineShaderStageCreateInfo stage{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
            stage.stage = VK_SHADER_STAGE_COMPUTE_BIT; stage.module = mod; stage.pName = "main";
            VkComputePipelineCreateInfo ci{ VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
            ci.stage = stage; ci.layout = layout;
            vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &ci, nullptr, &pipeline);
            vkDestroyShaderModule(device, mod, nullptr);
        };
        makeCompute("Assets/Shaders/Compiled/ClusterBuild.comp.spv", m_BuildLayout, m_BuildPipeline);
        makeCompute("Assets/Shaders/Compiled/ClusterCull.comp.spv",  m_CullLayout,  m_CullPipeline);
    }

    void ClusteredLighting::Reset()
    {
        m_PointLights.clear();
        m_SpotLights.clear();
    }

    void ClusteredLighting::SubmitPointLight(const PointLight& light)
    {
        if (m_PointLights.size() < MAX_POINT_LIGHTS) m_PointLights.push_back(light);
    }

    void ClusteredLighting::SubmitSpotLight(const SpotLight& light)
    {
        if (m_SpotLights.size() < MAX_SPOT_LIGHTS) m_SpotLights.push_back(light);
    }

    void ClusteredLighting::SetDirectionalLight(const DirectionalLight& light)
    {
        m_DirLight = light;
    }

    void ClusteredLighting::Flush(VkCommandBuffer cmd, const glm::mat4& proj, bool rebuildAABBs)
    {
        if (!m_Initialized) return;

        // Upload light data
        {
            VmaAllocationInfo info{};
            vmaGetAllocationInfo(VulkanDevice::Get().GetAllocator(), m_PointLightAlloc, &info);
            if (!m_PointLights.empty())
                memcpy(info.pMappedData, m_PointLights.data(), m_PointLights.size() * sizeof(PointLight));
            vmaGetAllocationInfo(VulkanDevice::Get().GetAllocator(), m_SpotLightAlloc, &info);
            if (!m_SpotLights.empty())
                memcpy(info.pMappedData, m_SpotLights.data(), m_SpotLights.size() * sizeof(SpotLight));
            vmaGetAllocationInfo(VulkanDevice::Get().GetAllocator(), m_DirLightAlloc, &info);
            memcpy(info.pMappedData, &m_DirLight, sizeof(DirectionalLight));
        }

        // Update cluster UBO
        if (m_ClusterUBOMapped)
        {
            ClusterUBO ubo{};
            ubo.InvProj       = glm::inverse(proj);
            ubo.ScreenNearFar = glm::vec4((f32)m_Desc.Width, (f32)m_Desc.Height, m_Desc.NearPlane, m_Desc.FarPlane);
            ubo.ClusterDims   = glm::uvec4(CLUSTER_X, CLUSTER_Y, CLUSTER_Z, 0);
            memcpy(m_ClusterUBOMapped, &ubo, sizeof(ubo));
        }

        VkMemoryBarrier barrier{ VK_STRUCTURE_TYPE_MEMORY_BARRIER };
        barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        if (rebuildAABBs)
        {
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_BuildPipeline);
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_BuildLayout, 0, 1, &m_CompDS, 0, nullptr);
            vkCmdDispatch(cmd, CLUSTER_X, CLUSTER_Y, CLUSTER_Z);
            vkCmdPipelineBarrier(cmd,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                0, 1, &barrier, 0, nullptr, 0, nullptr);
        }

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_CullPipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_CullLayout, 0, 1, &m_CompDS, 0, nullptr);
        vkCmdDispatch(cmd, CLUSTER_X, CLUSTER_Y, CLUSTER_Z);
        vkCmdPipelineBarrier(cmd,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0, 1, &barrier, 0, nullptr, 0, nullptr);
    }

    void ClusteredLighting::BindDescriptors(VkCommandBuffer cmd, VkPipelineLayout layout, u32 set)
    {
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, set, 1, &m_DS, 0, nullptr);
    }
}
