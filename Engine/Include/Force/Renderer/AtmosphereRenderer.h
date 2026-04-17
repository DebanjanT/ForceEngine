#pragma once
#include "Force/Core/Base.h"
#include "Force/Renderer/LightTypes.h"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>

namespace Force
{
    struct AtmosphereDesc
    {
        glm::vec3 RayleighScattering = { 5.8e-3f, 13.5e-3f, 33.1e-3f };
        f32 RayleighHeight   = 8.0f;
        f32 MieScattering    = 3.996e-3f;
        f32 MieHeight        = 1.2f;
        f32 MieAnisotropy    = 0.8f;
        f32 PlanetRadius     = 6360.0f;
        f32 AtmosphereRadius = 6460.0f;
        VkFormat ColorFormat = VK_FORMAT_B8G8R8A8_UNORM;
    };

    struct alignas(16) AtmosphereUBO
    {
        glm::vec4 SunDirection;       // xyz=dir, w=intensity
        glm::vec4 RayleighScatter;    // xyz=coeffs, w=height scale
        glm::vec4 MieParams;          // x=scatter, y=height, z=anisotropy, w=unused
        glm::vec4 PlanetRadii;        // x=planet, y=atmosphere, zw=unused
        glm::mat4 InvView;
        glm::mat4 InvProj;
    };

    class AtmosphereRenderer
    {
    public:
        static AtmosphereRenderer& Get();

        void Init(const AtmosphereDesc& desc = {});
        void Shutdown();

        void Render(VkCommandBuffer cmd,
                    const glm::mat4& invView, const glm::mat4& invProj,
                    const DirectionalLight& sun);

        bool IsInitialized() const { return m_Initialized; }

    private:
        void CreateUBO();
        void CreatePipeline();
        void CreateDescriptors();

    private:
        AtmosphereDesc m_Desc;
        bool           m_Initialized = false;

        VkBuffer      m_UBO       = VK_NULL_HANDLE;
        VmaAllocation m_UBOAlloc  = VK_NULL_HANDLE;
        void*         m_UBOMapped = nullptr;

        VkPipeline            m_Pipeline  = VK_NULL_HANDLE;
        VkPipelineLayout      m_Layout    = VK_NULL_HANDLE;
        VkDescriptorPool      m_Pool      = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_DSL       = VK_NULL_HANDLE;
        VkDescriptorSet       m_DS        = VK_NULL_HANDLE;
    };
}
