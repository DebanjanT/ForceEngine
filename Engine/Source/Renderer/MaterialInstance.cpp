#include "Force/Renderer/MaterialInstance.h"
#include "Force/Renderer/VulkanDevice.h"
#include "Force/Core/Logger.h"
#include <vk_mem_alloc.h>
#include <cstring>

namespace Force
{
    Ref<MaterialInstance> MaterialInstance::Create(const Ref<Material>& parent)
    {
        auto inst = CreateRef<MaterialInstance>();
        inst->Init(parent);
        return inst;
    }

    MaterialInstance::~MaterialInstance() { Destroy(); }

    void MaterialInstance::Init(const Ref<Material>& parent)
    {
        m_Parent = parent;
        m_Dirty  = true;

        // Create descriptor pool + set layout + set mirroring the parent's set 1
        VkDevice device = VulkanDevice::Get().GetDevice();

        VkDescriptorPoolSize sizes[] = {
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         1 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast<u32>(MaterialTextureSlot::Count) },
        };
        VkDescriptorPoolCreateInfo poolCI{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
        poolCI.maxSets = 1; poolCI.poolSizeCount = 2; poolCI.pPoolSizes = sizes;
        vkCreateDescriptorPool(device, &poolCI, nullptr, &m_Pool);

        // Build DSL matching parent's set 1 layout (UBO + textures)
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        bindings.push_back({ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT });
        for (u32 i = 0; i < static_cast<u32>(MaterialTextureSlot::Count); i++)
            bindings.push_back({ i + 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT });

        VkDescriptorSetLayoutCreateInfo dslCI{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
        dslCI.bindingCount = (u32)bindings.size(); dslCI.pBindings = bindings.data();
        vkCreateDescriptorSetLayout(device, &dslCI, nullptr, &m_DSL);

        VkDescriptorSetAllocateInfo allocI{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
        allocI.descriptorPool = m_Pool; allocI.descriptorSetCount = 1; allocI.pSetLayouts = &m_DSL;
        vkAllocateDescriptorSets(device, &allocI, &m_DS);

        // Create UBO (start as copy of parent)
        VkBufferCreateInfo bCI{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        bCI.size = sizeof(MaterialUBO); bCI.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        VmaAllocationCreateInfo aCI{}; aCI.usage = VMA_MEMORY_USAGE_CPU_TO_GPU; aCI.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
        VmaAllocationInfo info{};
        vmaCreateBuffer(VulkanDevice::Get().GetAllocator(), &bCI, &aCI, &m_UBO, &m_UBOAlloc, &info);
        m_UBOMapped = info.pMappedData;

        // Copy parent UBO data
        const MaterialUBO& parentUBO = parent->GetUBOData();
        memcpy(m_UBOMapped, &parentUBO, sizeof(MaterialUBO));

        // Copy parent textures as defaults
        for (u32 i = 0; i < static_cast<u32>(MaterialTextureSlot::Count); i++)
            m_TextureOverrides[i] = parent->GetTexture(static_cast<MaterialTextureSlot>(i));

        UpdateDescriptors();
    }

    void MaterialInstance::Destroy()
    {
        if (!m_Parent) return;
        VkDevice     device    = VulkanDevice::Get().GetDevice();
        VmaAllocator allocator = VulkanDevice::Get().GetAllocator();
        VulkanDevice::Get().WaitIdle();
        if (m_UBOMapped) vmaUnmapMemory(allocator, m_UBOAlloc);
        if (m_UBO)       vmaDestroyBuffer(allocator, m_UBO, m_UBOAlloc);
        if (m_DSL)       vkDestroyDescriptorSetLayout(device, m_DSL, nullptr);
        if (m_Pool)      vkDestroyDescriptorPool(device, m_Pool, nullptr);
        m_Parent = nullptr;
    }

    void MaterialInstance::SetFloat(const std::string& name, f32 value)
    {
        m_FloatOverrides[name] = value;
        m_Dirty = true;
    }

    void MaterialInstance::SetVector(const std::string& name, const glm::vec4& value)
    {
        m_VectorOverrides[name] = value;
        m_Dirty = true;
    }

    void MaterialInstance::SetTexture(MaterialTextureSlot slot, const Ref<Texture2D>& texture)
    {
        m_TextureOverrides[static_cast<u32>(slot)] = texture;
        m_Dirty = true;
    }

    void MaterialInstance::ResetFloat(const std::string& name)  { m_FloatOverrides.erase(name); m_Dirty = true; }
    void MaterialInstance::ResetVector(const std::string& name) { m_VectorOverrides.erase(name); m_Dirty = true; }

    void MaterialInstance::RebuildUBO()
    {
        // Start from parent UBO
        MaterialUBO ubo = m_Parent->GetUBOData();

        // Apply overrides by name
        if (auto it = m_VectorOverrides.find("AlbedoColor"); it != m_VectorOverrides.end()) ubo.AlbedoColor = it->second;
        if (auto it = m_FloatOverrides.find("Metallic");     it != m_FloatOverrides.end())  ubo.Metallic = it->second;
        if (auto it = m_FloatOverrides.find("Roughness");    it != m_FloatOverrides.end())  ubo.Roughness = it->second;
        if (auto it = m_FloatOverrides.find("AO");           it != m_FloatOverrides.end())  ubo.AO = it->second;
        if (auto it = m_FloatOverrides.find("EmissiveStrength"); it != m_FloatOverrides.end()) ubo.EmissiveStrength = it->second;
        if (auto it = m_VectorOverrides.find("EmissiveColor"); it != m_VectorOverrides.end()) ubo.EmissiveColor = it->second;

        memcpy(m_UBOMapped, &ubo, sizeof(MaterialUBO));
    }

    void MaterialInstance::UpdateDescriptors()
    {
        // UBO
        VkDescriptorBufferInfo bufInfo{ m_UBO, 0, sizeof(MaterialUBO) };
        VkWriteDescriptorSet uboWrite{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        uboWrite.dstSet = m_DS; uboWrite.dstBinding = 0; uboWrite.descriptorCount = 1;
        uboWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; uboWrite.pBufferInfo = &bufInfo;
        vkUpdateDescriptorSets(VulkanDevice::Get().GetDevice(), 1, &uboWrite, 0, nullptr);

        // Textures — update only those that are set
        for (u32 i = 0; i < static_cast<u32>(MaterialTextureSlot::Count); i++)
        {
            if (!m_TextureOverrides[i]) continue;
            // Texture2D must expose GetImageView/GetSampler — using existing Material pattern
            // (actual descriptor update would call m_TextureOverrides[i]->GetDescriptorInfo())
        }
    }

    void MaterialInstance::Apply()
    {
        if (!m_Dirty) return;
        RebuildUBO();
        UpdateDescriptors();
        m_Dirty = false;
    }

    void MaterialInstance::Bind(VkCommandBuffer cmd, VkPipelineLayout layout) const
    {
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 1, 1, &m_DS, 0, nullptr);
    }
}
