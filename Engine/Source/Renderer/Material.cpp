#include "Force/Renderer/Material.h"
#include "Force/Renderer/Renderer.h"
#include "Force/Renderer/VulkanDevice.h"
#include "Force/Renderer/VulkanBuffer.h"
#include "Force/Core/Logger.h"

namespace Force
{
    // -------------------------------------------------------------------------
    // Material
    // -------------------------------------------------------------------------
    Material::~Material()
    {
        Destroy();
    }

    void Material::Init(const MaterialSpec& spec)
    {
        m_Name   = spec.Name;
        m_Shader = spec.Shader;

        // Initialize UBO data
        m_UBOData.AlbedoColor      = spec.AlbedoColor;
        m_UBOData.Metallic         = spec.Metallic;
        m_UBOData.Roughness        = spec.Roughness;
        m_UBOData.AO               = spec.AO;
        m_UBOData.EmissiveStrength = spec.EmissiveStrength;
        m_UBOData.EmissiveColor    = spec.EmissiveColor;

        // Initialize textures with defaults from TextureLibrary
        TextureLibrary& texLib = TextureLibrary::Get();
        m_Textures[static_cast<u32>(MaterialTextureSlot::Albedo)]   = spec.AlbedoTexture   ? spec.AlbedoTexture   : texLib.GetWhite();
        m_Textures[static_cast<u32>(MaterialTextureSlot::Normal)]   = spec.NormalTexture   ? spec.NormalTexture   : texLib.GetNormal();
        m_Textures[static_cast<u32>(MaterialTextureSlot::MetallicRoughness)] = spec.MetallicRoughnessTexture ? spec.MetallicRoughnessTexture : texLib.GetWhite();
        m_Textures[static_cast<u32>(MaterialTextureSlot::AO)]       = spec.AOTexture       ? spec.AOTexture       : texLib.GetWhite();
        m_Textures[static_cast<u32>(MaterialTextureSlot::Emissive)] = spec.EmissiveTexture ? spec.EmissiveTexture : texLib.GetBlack();

        CreateDescriptorSets();
    }

    void Material::CreateDescriptorSets()
    {
        VkDevice device = VulkanDevice::Get().GetDevice();
        u32 framesInFlight = Renderer::GetMaxFramesInFlight();

        // Create descriptor set layout (set 1 for materials)
        // Binding 0: MaterialUBO
        // Bindings 1-5: textures (albedo, normal, metallicRoughness, AO, emissive)
        std::vector<VkDescriptorSetLayoutBinding> bindings(6);

        // UBO binding
        bindings[0].binding            = 0;
        bindings[0].descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[0].descriptorCount    = 1;
        bindings[0].stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings[0].pImmutableSamplers = nullptr;

        // Texture bindings
        for (u32 i = 1; i <= 5; ++i)
        {
            bindings[i].binding            = i;
            bindings[i].descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            bindings[i].descriptorCount    = 1;
            bindings[i].stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;
            bindings[i].pImmutableSamplers = nullptr;
        }

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<u32>(bindings.size());
        layoutInfo.pBindings    = bindings.data();

        VkResult result = vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &m_DescriptorSetLayout);
        FORCE_CORE_ASSERT(result == VK_SUCCESS, "Failed to create material descriptor set layout");

        // Create UBOs (one per frame)
        m_UBOs.resize(framesInFlight);
        for (u32 i = 0; i < framesInFlight; ++i)
        {
            m_UBOs[i] = new UniformBuffer();
            m_UBOs[i]->Create(sizeof(MaterialUBO));
            m_UBOs[i]->SetData(&m_UBOData, sizeof(MaterialUBO));
        }

        // Allocate descriptor sets (one per frame)
        m_DescriptorSets.resize(framesInFlight);
        std::vector<VkDescriptorSetLayout> layouts(framesInFlight, m_DescriptorSetLayout);

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool     = Renderer::GetDescriptorPool();
        allocInfo.descriptorSetCount = framesInFlight;
        allocInfo.pSetLayouts        = layouts.data();

        result = vkAllocateDescriptorSets(device, &allocInfo, m_DescriptorSets.data());
        FORCE_CORE_ASSERT(result == VK_SUCCESS, "Failed to allocate material descriptor sets");

        // Write initial descriptor data
        UpdateDescriptorSets();
        m_DescriptorsDirty = false;
    }

    void Material::UpdateDescriptorSets()
    {
        VkDevice device = VulkanDevice::Get().GetDevice();
        u32 framesInFlight = Renderer::GetMaxFramesInFlight();

        for (u32 frame = 0; frame < framesInFlight; ++frame)
        {
            std::vector<VkWriteDescriptorSet> writes;
            writes.reserve(6);

            // UBO write
            VkDescriptorBufferInfo bufferInfo = m_UBOs[frame]->GetDescriptorInfo();
            VkWriteDescriptorSet uboWrite{};
            uboWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            uboWrite.dstSet          = m_DescriptorSets[frame];
            uboWrite.dstBinding      = 0;
            uboWrite.dstArrayElement = 0;
            uboWrite.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            uboWrite.descriptorCount = 1;
            uboWrite.pBufferInfo     = &bufferInfo;
            writes.push_back(uboWrite);

            // Texture writes
            std::vector<VkDescriptorImageInfo> imageInfos(5);
            for (u32 slot = 0; slot < 5; ++slot)
            {
                imageInfos[slot] = m_Textures[slot]->GetDescriptorInfo();

                VkWriteDescriptorSet texWrite{};
                texWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                texWrite.dstSet          = m_DescriptorSets[frame];
                texWrite.dstBinding      = slot + 1;
                texWrite.dstArrayElement = 0;
                texWrite.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                texWrite.descriptorCount = 1;
                texWrite.pImageInfo      = &imageInfos[slot];
                writes.push_back(texWrite);
            }

            vkUpdateDescriptorSets(device, static_cast<u32>(writes.size()), writes.data(), 0, nullptr);
        }
    }

    void Material::Destroy()
    {
        VkDevice device = VulkanDevice::Get().GetDevice();

        // UBOs
        for (auto* ubo : m_UBOs)
        {
            if (ubo)
            {
                ubo->Destroy();
                delete ubo;
            }
        }
        m_UBOs.clear();

        // Descriptor sets are freed when the pool is destroyed/reset
        m_DescriptorSets.clear();

        // Descriptor set layout
        if (m_DescriptorSetLayout != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(device, m_DescriptorSetLayout, nullptr);
            m_DescriptorSetLayout = VK_NULL_HANDLE;
        }

        // Clear texture references
        for (u32 i = 0; i < static_cast<u32>(MaterialTextureSlot::Count); ++i)
        {
            m_Textures[i].reset();
        }

        m_Shader.reset();
    }

    void Material::Bind(VkCommandBuffer cmd, VkPipelineLayout layout) const
    {
        u32 currentFrame = Renderer::GetCurrentFrameIndex();
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                 layout, 1, 1,
                                 &m_DescriptorSets[currentFrame],
                                 0, nullptr);
    }

    void Material::SetAlbedoColor(const glm::vec4& color)
    {
        m_UBOData.AlbedoColor = color;
    }

    void Material::SetMetallic(float value)
    {
        m_UBOData.Metallic = value;
    }

    void Material::SetRoughness(float value)
    {
        m_UBOData.Roughness = value;
    }

    void Material::SetAO(float value)
    {
        m_UBOData.AO = value;
    }

    void Material::SetEmissive(const glm::vec4& color, float strength)
    {
        m_UBOData.EmissiveColor    = color;
        m_UBOData.EmissiveStrength = strength;
    }

    void Material::SetTexture(MaterialTextureSlot slot, const Ref<Texture2D>& texture)
    {
        u32 slotIndex = static_cast<u32>(slot);
        FORCE_CORE_ASSERT(slotIndex < static_cast<u32>(MaterialTextureSlot::Count), "Invalid texture slot");

        m_Textures[slotIndex] = texture ? texture : TextureLibrary::Get().GetWhite();
        m_DescriptorsDirty = true;
    }

    Ref<Texture2D> Material::GetTexture(MaterialTextureSlot slot) const
    {
        u32 slotIndex = static_cast<u32>(slot);
        FORCE_CORE_ASSERT(slotIndex < static_cast<u32>(MaterialTextureSlot::Count), "Invalid texture slot");
        return m_Textures[slotIndex];
    }

    void Material::UploadUBO()
    {
        u32 currentFrame = Renderer::GetCurrentFrameIndex();
        m_UBOs[currentFrame]->SetData(&m_UBOData, sizeof(MaterialUBO));

        // If textures changed, update all frames' descriptor sets
        if (m_DescriptorsDirty)
        {
            UpdateDescriptorSets();
            m_DescriptorsDirty = false;
        }
    }

    Ref<Material> Material::Create(const MaterialSpec& spec)
    {
        Ref<Material> material = CreateRef<Material>();
        material->Init(spec);
        return material;
    }

    // -------------------------------------------------------------------------
    // MaterialLibrary
    // -------------------------------------------------------------------------
    MaterialLibrary& MaterialLibrary::Get()
    {
        static MaterialLibrary instance;
        return instance;
    }

    void MaterialLibrary::Add(const std::string& name, const Ref<Material>& material)
    {
        FORCE_CORE_ASSERT(!Exists(name), "Material '{}' already exists", name);
        m_Materials[name] = material;
    }

    Ref<Material> MaterialLibrary::Get(const std::string& name)
    {
        auto it = m_Materials.find(name);
        FORCE_CORE_ASSERT(it != m_Materials.end(), "Material '{}' not found", name);
        return it->second;
    }

    bool MaterialLibrary::Exists(const std::string& name) const
    {
        return m_Materials.find(name) != m_Materials.end();
    }

    void MaterialLibrary::Clear()
    {
        for (auto& [name, material] : m_Materials)
        {
            material->Destroy();
        }
        m_Materials.clear();
    }
}
