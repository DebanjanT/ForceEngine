#pragma once
#include "Force/Core/Base.h"
#include "Force/Renderer/Material.h"
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>

namespace Force
{
    // MaterialInstance overrides scalar/vector/texture params on a parent material.
    // Creates its own descriptor set — the parent's pipeline layout stays unchanged.
    class MaterialInstance
    {
    public:
        MaterialInstance() = default;
        ~MaterialInstance();

        void Init(const Ref<Material>& parent);
        void Destroy();

        // Parameter overrides (name must match a uniform in the parent shader)
        void SetFloat(const std::string& name, f32 value);
        void SetVector(const std::string& name, const glm::vec4& value);
        void SetTexture(MaterialTextureSlot slot, const Ref<Texture2D>& texture);

        // Reset a param back to parent's value
        void ResetFloat(const std::string& name);
        void ResetVector(const std::string& name);

        // Apply overrides and upload to GPU (call once per frame if dirty)
        void Apply();

        // Bind this instance's descriptor set to the command buffer
        void Bind(VkCommandBuffer cmd, VkPipelineLayout layout) const;

        const Ref<Material>& GetParent() const { return m_Parent; }
        bool                 IsDirty()   const { return m_Dirty; }

        static Ref<MaterialInstance> Create(const Ref<Material>& parent);

    private:
        void RebuildUBO();
        void UpdateDescriptors();

    private:
        Ref<Material> m_Parent;
        bool          m_Dirty = true;

        std::unordered_map<std::string, f32>        m_FloatOverrides;
        std::unordered_map<std::string, glm::vec4>  m_VectorOverrides;
        Ref<Texture2D> m_TextureOverrides[static_cast<u32>(MaterialTextureSlot::Count)];

        // Per-instance GPU resources
        VkDescriptorSet       m_DS     = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_DSL    = VK_NULL_HANDLE;
        VkDescriptorPool      m_Pool   = VK_NULL_HANDLE;
        VkBuffer              m_UBO    = VK_NULL_HANDLE;
        VmaAllocation         m_UBOAlloc = VK_NULL_HANDLE;
        void*                 m_UBOMapped = nullptr;
    };
}
