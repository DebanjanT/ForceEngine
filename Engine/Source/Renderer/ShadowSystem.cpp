#include "Force/Renderer/ShadowSystem.h"
#include "Force/Renderer/VulkanDevice.h"
#include "Force/Core/Logger.h"
#include <vk_mem_alloc.h>
#include <glm/gtc/matrix_transform.hpp>
#include <cstring>
#include <cfloat>

namespace Force
{
    ShadowSystem& ShadowSystem::Get()
    {
        static ShadowSystem instance;
        return instance;
    }

    void ShadowSystem::Init(const ShadowMapDesc& desc)
    {
        if (m_Initialized) Shutdown();
        m_Desc = desc;
        CreateDepthResources();
        CreateRenderPass();
        CreateFramebuffers();
        CreateUBO();
        CreateSampler();
        m_Initialized = true;
        FORCE_CORE_INFO("ShadowSystem: {} cascades @ {}x{}", desc.CascadeCount, desc.Resolution, desc.Resolution);
    }

    void ShadowSystem::Shutdown()
    {
        if (!m_Initialized) return;
        VkDevice     device    = VulkanDevice::Get().GetDevice();
        VmaAllocator allocator = VulkanDevice::Get().GetAllocator();
        VulkanDevice::Get().WaitIdle();

        if (m_UBOMapped) vmaUnmapMemory(allocator, m_UBOAlloc);
        if (m_UBO)       vmaDestroyBuffer(allocator, m_UBO, m_UBOAlloc);
        if (m_Sampler)   vkDestroySampler(device, m_Sampler, nullptr);

        for (u32 i = 0; i < m_Desc.CascadeCount; i++)
        {
            if (m_Framebuffers[i]) vkDestroyFramebuffer(device, m_Framebuffers[i], nullptr);
            if (m_Views[i])        vkDestroyImageView(device, m_Views[i], nullptr);
            if (m_Images[i])       vmaDestroyImage(allocator, m_Images[i], m_Allocs[i]);
        }
        if (m_RenderPass) vkDestroyRenderPass(device, m_RenderPass, nullptr);
        m_Initialized = false;
    }

    void ShadowSystem::CreateDepthResources()
    {
        VkImageCreateInfo imageCI{};
        imageCI.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCI.imageType     = VK_IMAGE_TYPE_2D;
        imageCI.format        = VK_FORMAT_D32_SFLOAT;
        imageCI.extent        = { m_Desc.Resolution, m_Desc.Resolution, 1 };
        imageCI.mipLevels     = 1;
        imageCI.arrayLayers   = 1;
        imageCI.samples       = VK_SAMPLE_COUNT_1_BIT;
        imageCI.tiling        = VK_IMAGE_TILING_OPTIMAL;
        imageCI.usage         = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VmaAllocationCreateInfo allocCI{};
        allocCI.usage = VMA_MEMORY_USAGE_GPU_ONLY;

        VkImageViewCreateInfo viewCI{};
        viewCI.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewCI.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        viewCI.format                          = VK_FORMAT_D32_SFLOAT;
        viewCI.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
        viewCI.subresourceRange.levelCount     = 1;
        viewCI.subresourceRange.layerCount     = 1;

        for (u32 i = 0; i < m_Desc.CascadeCount; i++)
        {
            vmaCreateImage(VulkanDevice::Get().GetAllocator(), &imageCI, &allocCI, &m_Images[i], &m_Allocs[i], nullptr);
            viewCI.image = m_Images[i];
            vkCreateImageView(VulkanDevice::Get().GetDevice(), &viewCI, nullptr, &m_Views[i]);
        }
    }

    void ShadowSystem::CreateRenderPass()
    {
        VkAttachmentDescription depth{};
        depth.format         = VK_FORMAT_D32_SFLOAT;
        depth.samples        = VK_SAMPLE_COUNT_1_BIT;
        depth.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depth.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
        depth.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depth.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
        depth.finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkAttachmentReference depthRef{ 0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

        VkSubpassDescription sub{};
        sub.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
        sub.pDepthStencilAttachment = &depthRef;

        VkSubpassDependency deps[2]{};
        deps[0] = { VK_SUBPASS_EXTERNAL, 0,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            VK_ACCESS_SHADER_READ_BIT,                    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, 0 };
        deps[1] = { 0, VK_SUBPASS_EXTERNAL,
            VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,   VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, 0 };

        VkRenderPassCreateInfo rpCI{};
        rpCI.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        rpCI.attachmentCount = 1;  rpCI.pAttachments    = &depth;
        rpCI.subpassCount    = 1;  rpCI.pSubpasses      = &sub;
        rpCI.dependencyCount = 2;  rpCI.pDependencies   = deps;
        vkCreateRenderPass(VulkanDevice::Get().GetDevice(), &rpCI, nullptr, &m_RenderPass);
    }

    void ShadowSystem::CreateFramebuffers()
    {
        for (u32 i = 0; i < m_Desc.CascadeCount; i++)
        {
            VkFramebufferCreateInfo fbCI{};
            fbCI.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            fbCI.renderPass      = m_RenderPass;
            fbCI.attachmentCount = 1;
            fbCI.pAttachments    = &m_Views[i];
            fbCI.width = fbCI.height = m_Desc.Resolution;
            fbCI.layers = 1;
            vkCreateFramebuffer(VulkanDevice::Get().GetDevice(), &fbCI, nullptr, &m_Framebuffers[i]);
        }
    }

    void ShadowSystem::CreateUBO()
    {
        VkBufferCreateInfo bCI{};
        bCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bCI.size  = sizeof(CascadeData);
        bCI.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

        VmaAllocationCreateInfo aCI{};
        aCI.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        aCI.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VmaAllocationInfo info{};
        vmaCreateBuffer(VulkanDevice::Get().GetAllocator(), &bCI, &aCI, &m_UBO, &m_UBOAlloc, &info);
        m_UBOMapped = info.pMappedData;
    }

    void ShadowSystem::CreateSampler()
    {
        VkSamplerCreateInfo sCI{};
        sCI.sType         = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sCI.magFilter     = VK_FILTER_LINEAR;
        sCI.minFilter     = VK_FILTER_LINEAR;
        sCI.addressModeU  = sCI.addressModeV = sCI.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        sCI.borderColor   = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        sCI.compareEnable = VK_TRUE;
        sCI.compareOp     = VK_COMPARE_OP_LESS_OR_EQUAL;
        sCI.mipmapMode    = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        vkCreateSampler(VulkanDevice::Get().GetDevice(), &sCI, nullptr, &m_Sampler);
    }

    void ShadowSystem::UpdateCascades(const DirectionalLight& sun,
                                       const glm::mat4& camView, const glm::mat4& camProj,
                                       f32 camNear, f32 camFar)
    {
        glm::vec3 lightDir = glm::normalize(glm::vec3(sun.Direction));
        glm::mat4 invCam   = glm::inverse(camProj * camView);

        // Practical split scheme
        float splits[MAX_SHADOW_CASCADES + 1];
        splits[0] = camNear;
        float range = camFar - camNear;
        float ratio = camFar / camNear;
        for (u32 i = 1; i <= m_Desc.CascadeCount; i++)
        {
            float p   = (float)i / (float)m_Desc.CascadeCount;
            float log = camNear * glm::pow(ratio, p);
            float uni = camNear + range * p;
            splits[i] = m_Desc.LambdaSplit * log + (1.0f - m_Desc.LambdaSplit) * uni;
        }

        for (u32 i = 0; i < m_Desc.CascadeCount; i++)
        {
            float n = splits[i], f = splits[i + 1];

            // Compute NDC z range for this slice
            glm::vec4 nc = camProj * glm::vec4(0, 0, -n, 1); nc /= nc.w;
            glm::vec4 fc = camProj * glm::vec4(0, 0, -f, 1); fc /= fc.w;

            glm::vec4 corners[8] = {
                {-1,-1, nc.z, 1}, { 1,-1, nc.z, 1}, {-1, 1, nc.z, 1}, { 1, 1, nc.z, 1},
                {-1,-1, fc.z, 1}, { 1,-1, fc.z, 1}, {-1, 1, fc.z, 1}, { 1, 1, fc.z, 1},
            };

            glm::vec3 center(0.0f);
            for (auto& c : corners) { glm::vec4 w = invCam * c; w /= w.w; c = w; center += glm::vec3(w); }
            center /= 8.0f;

            glm::mat4 lightView = glm::lookAt(center - lightDir, center, glm::vec3(0, 1, 0));

            glm::vec3 mn(FLT_MAX), mx(-FLT_MAX);
            for (auto& c : corners)
            {
                glm::vec3 ls = glm::vec3(lightView * glm::vec4(glm::vec3(c), 1.0f));
                mn = glm::min(mn, ls);
                mx = glm::max(mx, ls);
            }
            // Extend Z to capture casters behind the frustum
            mn.z -= (mx.z - mn.z) * 2.0f;

            glm::mat4 lightProj = glm::ortho(mn.x, mx.x, mn.y, mx.y, mn.z, mx.z);
            lightProj[1][1] *= -1.0f; // Vulkan Y flip

            m_CascadeData.ViewProj[i]     = lightProj * lightView;
            m_CascadeData.SplitDepths[i]  = splits[i + 1];
        }

        if (m_UBOMapped) memcpy(m_UBOMapped, &m_CascadeData, sizeof(CascadeData));
    }

    void ShadowSystem::BeginCascade(VkCommandBuffer cmd, u32 idx)
    {
        VkClearValue clear{};
        clear.depthStencil = { 1.0f, 0 };

        VkRenderPassBeginInfo rpBI{};
        rpBI.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rpBI.renderPass        = m_RenderPass;
        rpBI.framebuffer       = m_Framebuffers[idx];
        rpBI.renderArea.extent = { m_Desc.Resolution, m_Desc.Resolution };
        rpBI.clearValueCount   = 1;
        rpBI.pClearValues      = &clear;
        vkCmdBeginRenderPass(cmd, &rpBI, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport vp{ 0, 0, (f32)m_Desc.Resolution, (f32)m_Desc.Resolution, 0, 1 };
        VkRect2D   sc{ {0,0}, {m_Desc.Resolution, m_Desc.Resolution} };
        vkCmdSetViewport(cmd, 0, 1, &vp);
        vkCmdSetScissor(cmd, 0, 1, &sc);
    }

    void ShadowSystem::EndCascade(VkCommandBuffer cmd)
    {
        vkCmdEndRenderPass(cmd);
    }
}
