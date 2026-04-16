#include "Force/Renderer/DecalSystem.h"
#include "Force/Renderer/Renderer.h"
#include "Force/Core/Logger.h"
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

namespace Force
{
    DecalSystem& DecalSystem::Get()
    {
        static DecalSystem instance;
        return instance;
    }

    void DecalSystem::Init(u32 maxDecals)
    {
        if (m_Initialized) return;
        m_MaxDecals = maxDecals;

        m_DecalBuffer.Create(
            sizeof(DecalGPU) * maxDecals,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU);

        CreatePipeline();
        m_Initialized = true;
        FORCE_CORE_INFO("DecalSystem initialized (max: {})", maxDecals);
    }

    void DecalSystem::Shutdown()
    {
        if (!m_Initialized) return;
        VkDevice dev = Renderer::GetDevice().GetDevice();

        if (m_Pipeline)       { vkDestroyPipeline(dev, m_Pipeline, nullptr);             m_Pipeline = VK_NULL_HANDLE; }
        if (m_PipelineLayout) { vkDestroyPipelineLayout(dev, m_PipelineLayout, nullptr); m_PipelineLayout = VK_NULL_HANDLE; }
        if (m_DSL)            { vkDestroyDescriptorSetLayout(dev, m_DSL, nullptr);       m_DSL = VK_NULL_HANDLE; }

        m_DecalBuffer.Destroy();
        m_Decals.clear();
        m_Initialized = false;
    }

    u32 DecalSystem::AddDecal(const DecalDesc& desc)
    {
        if (m_Decals.size() >= m_MaxDecals)
        {
            FORCE_CORE_WARN("DecalSystem: max decal count reached");
            return UINT32_MAX;
        }
        DecalInstance inst;
        inst.Desc = desc;
        inst.Id   = m_NextId++;
        m_Decals.push_back(std::move(inst));
        return m_Decals.back().Id;
    }

    void DecalSystem::RemoveDecal(u32 id)
    {
        m_Decals.erase(std::remove_if(m_Decals.begin(), m_Decals.end(),
            [id](const DecalInstance& d){ return d.Id == id; }), m_Decals.end());
    }

    void DecalSystem::Clear()
    {
        m_Decals.clear();
    }

    void DecalSystem::Update(f32 dt)
    {
        for (auto& d : m_Decals)
        {
            if (d.Desc.Lifetime < 0.0f) continue;

            d.Age += dt;
            if (d.Age >= d.Desc.Lifetime && !d.FadingOut)
                d.FadingOut = true;

            if (d.FadingOut && d.Desc.FadeOutTime > 1e-6f)
            {
                f32 fadeElapsed = d.Age - d.Desc.Lifetime;
                d.FadeAlpha = 1.0f - glm::clamp(fadeElapsed / d.Desc.FadeOutTime, 0.0f, 1.0f);
            }
        }

        // Remove fully faded decals
        m_Decals.erase(std::remove_if(m_Decals.begin(), m_Decals.end(),
            [](const DecalInstance& d){ return d.FadingOut && d.FadeAlpha <= 0.001f; }),
            m_Decals.end());
    }

    u32 DecalSystem::GetActiveCount() const
    {
        return static_cast<u32>(m_Decals.size());
    }

    void DecalSystem::UploadDecalBuffer()
    {
        if (m_Decals.empty()) return;

        std::vector<DecalGPU> gpuData;
        gpuData.reserve(m_Decals.size());

        for (const auto& d : m_Decals)
        {
            DecalGPU gpu{};
            // Inverse world matrix for decal projection
            glm::mat4 world = glm::translate(glm::mat4(1.0f), d.Desc.Position);
            world *= glm::mat4_cast(d.Desc.Rotation);
            world = glm::scale(world, d.Desc.HalfExtents * 2.0f);
            gpu.InverseWorld = glm::inverse(world);

            gpu.Color  = d.Desc.Color;
            gpu.Color.a *= d.Desc.Opacity * d.FadeAlpha;
            gpu.Opacity = gpu.Color.a;
            gpuData.push_back(gpu);
        }

        m_DecalBuffer.SetData(gpuData.data(),
                              gpuData.size() * sizeof(DecalGPU));
    }

    void DecalSystem::Render(VkCommandBuffer cmd,
                              VkImageView     /*gbufferDepth*/,
                              VkImageView     /*gbufferAlbedo*/,
                              VkImageView     /*gbufferNormal*/,
                              const glm::mat4& /*invViewProj*/,
                              const glm::vec3& /*cameraPos*/)
    {
        if (!m_Initialized || m_Decals.empty() || m_Pipeline == VK_NULL_HANDLE) return;

        UploadDecalBuffer();

        // Sort by priority (higher draws last = on top)
        std::sort(m_Decals.begin(), m_Decals.end(),
            [](const DecalInstance& a, const DecalInstance& b){
                return a.Desc.SortPriority < b.Desc.SortPriority;
            });

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                 m_PipelineLayout, 0, 1, &m_DS, 0, nullptr);

        // Draw a full-screen quad for each decal batch
        // (proper implementation uses a unit cube per decal with invViewProj push constant)
        u32 count = static_cast<u32>(m_Decals.size());
        vkCmdDraw(cmd, 6, count, 0, 0);
    }

    void DecalSystem::CreatePipeline()
    {
        // Descriptor set layout: 0=SSBO decals, 1=GBuffer depth, 2=albedo tex, 3=normal tex
        std::array<VkDescriptorSetLayoutBinding, 4> bindings{};
        bindings[0] = { 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,          1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
        bindings[1] = { 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,  1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
        bindings[2] = { 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,  1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
        bindings[3] = { 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,  1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };

        VkDescriptorSetLayoutCreateInfo dslInfo{};
        dslInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        dslInfo.bindingCount = static_cast<u32>(bindings.size());
        dslInfo.pBindings    = bindings.data();
        vkCreateDescriptorSetLayout(Renderer::GetDevice().GetDevice(), &dslInfo, nullptr, &m_DSL);

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool     = Renderer::GetDescriptorPool();
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts        = &m_DSL;
        vkAllocateDescriptorSets(Renderer::GetDevice().GetDevice(), &allocInfo, &m_DS);

        // Write SSBO binding
        VkDescriptorBufferInfo bufInfo = { m_DecalBuffer.GetBuffer(), 0, VK_WHOLE_SIZE };
        VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr,
            m_DS, 0, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &bufInfo, nullptr };
        vkUpdateDescriptorSets(Renderer::GetDevice().GetDevice(), 1, &write, 0, nullptr);

        // Pipeline layout with push constant: mat4 invViewProj + vec4 camPos
        VkPushConstantRange pc{};
        pc.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pc.size       = sizeof(glm::mat4) + sizeof(glm::vec4);

        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.setLayoutCount         = 1;
        layoutInfo.pSetLayouts            = &m_DSL;
        layoutInfo.pushConstantRangeCount = 1;
        layoutInfo.pPushConstantRanges    = &pc;
        vkCreatePipelineLayout(Renderer::GetDevice().GetDevice(), &layoutInfo, nullptr, &m_PipelineLayout);

        FORCE_CORE_INFO("DecalSystem: pipeline layout created (load Decal.vert/frag.spv to complete)");
    }
}
