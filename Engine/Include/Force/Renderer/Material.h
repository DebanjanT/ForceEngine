#pragma once

#include "Force/Core/Base.h"
#include "Force/Renderer/Shader.h"
#include "Force/Renderer/Texture.h"
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>

namespace Force
{
    // PBR material uniform data (set 1, binding 0)
    struct MaterialUBO
    {
        alignas(16) glm::vec4 AlbedoColor    = glm::vec4(1.0f);
        alignas(4)  float     Metallic       = 0.0f;
        alignas(4)  float     Roughness      = 0.5f;
        alignas(4)  float     AO             = 1.0f;
        alignas(4)  float     EmissiveStrength = 0.0f;
        alignas(16) glm::vec4 EmissiveColor  = glm::vec4(0.0f);
    };

    // Texture slots for PBR workflow
    enum class MaterialTextureSlot : u32
    {
        Albedo         = 0,
        Normal         = 1,
        MetallicRoughness = 2,  // packed: R = metallic, G = roughness
        AO             = 3,
        Emissive       = 4,
        Count
    };

    // Material specification for creating materials
    struct MaterialSpec
    {
        std::string       Name;
        Ref<Shader>       Shader;
        glm::vec4         AlbedoColor         = glm::vec4(1.0f);
        float             Metallic            = 0.0f;
        float             Roughness           = 0.5f;
        float             AO                  = 1.0f;
        float             EmissiveStrength    = 0.0f;
        glm::vec4         EmissiveColor       = glm::vec4(0.0f);
        Ref<Texture2D>    AlbedoTexture       = nullptr;
        Ref<Texture2D>    NormalTexture       = nullptr;
        Ref<Texture2D>    MetallicRoughnessTexture = nullptr;
        Ref<Texture2D>    AOTexture           = nullptr;
        Ref<Texture2D>    EmissiveTexture     = nullptr;
    };

    class Material
    {
    public:
        Material() = default;
        ~Material();

        void Init(const MaterialSpec& spec);
        void Destroy();

        // Bind material descriptor set (set 1) to the command buffer
        void Bind(VkCommandBuffer cmd, VkPipelineLayout layout) const;

        // Update uniform data
        void SetAlbedoColor(const glm::vec4& color);
        void SetMetallic(float value);
        void SetRoughness(float value);
        void SetAO(float value);
        void SetEmissive(const glm::vec4& color, float strength);

        // Update textures (requires descriptor set update)
        void SetTexture(MaterialTextureSlot slot, const Ref<Texture2D>& texture);

        // Getters
        const std::string& GetName() const { return m_Name; }
        Ref<Shader>        GetShader() const { return m_Shader; }
        const MaterialUBO& GetUBOData() const { return m_UBOData; }
        Ref<Texture2D>     GetTexture(MaterialTextureSlot slot) const;

        // Upload current UBO data to GPU
        void UploadUBO();

        // Factory method
        static Ref<Material> Create(const MaterialSpec& spec);

    private:
        void CreateDescriptorSets();
        void UpdateDescriptorSets();

    private:
        std::string    m_Name;
        Ref<Shader>    m_Shader;
        MaterialUBO    m_UBOData;

        // Per-frame-in-flight resources
        std::vector<VkDescriptorSet> m_DescriptorSets;  // one per frame
        std::vector<class UniformBuffer*> m_UBOs;       // one per frame

        // Textures (shared across frames)
        Ref<Texture2D> m_Textures[static_cast<u32>(MaterialTextureSlot::Count)];

        VkDescriptorSetLayout m_DescriptorSetLayout = VK_NULL_HANDLE;
        bool m_DescriptorsDirty = true;
    };

    // Material library for managing named materials
    class MaterialLibrary
    {
    public:
        static MaterialLibrary& Get();

        void           Add(const std::string& name, const Ref<Material>& material);
        Ref<Material>  Get(const std::string& name);
        bool           Exists(const std::string& name) const;
        void           Clear();

    private:
        MaterialLibrary() = default;
        std::unordered_map<std::string, Ref<Material>> m_Materials;
    };
}
