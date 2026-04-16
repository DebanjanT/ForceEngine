#include "Force/Renderer/ParticleSystem.h"
#include "Force/Renderer/Renderer.h"
#include "Force/Core/Logger.h"
#include <algorithm>
#include <cstring>

namespace Force
{
    // -------------------------------------------------------------------------
    // ParticleEmitter

    ParticleEmitter::~ParticleEmitter()
    {
        Destroy();
    }

    void ParticleEmitter::Create(const ParticleEmitterDesc& desc)
    {
        m_Desc = desc;

        // Particle buffer: MaxParticles * sizeof(ParticleGPU)
        usize particleBufSize = desc.MaxParticles * sizeof(ParticleGPU);
        m_ParticleBuffer.Create(particleBufSize,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY);

        // Compute UBO
        m_ComputeUBO.Create(sizeof(ParticleComputeUBO),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU);

        // Indirect draw args: VkDrawIndirectCommand
        m_IndirectBuffer.Create(sizeof(VkDrawIndirectCommand),
            VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU);

        // Initialise indirect draw to 0 vertices
        VkDrawIndirectCommand drawCmd{};
        drawCmd.vertexCount   = 0;
        drawCmd.instanceCount = 1;
        m_IndirectBuffer.SetData(&drawCmd, sizeof(drawCmd));

        CreateDescriptors();
        CreateComputePipeline();
        CreateRenderPipeline();
    }

    void ParticleEmitter::Destroy()
    {
        VkDevice dev = Renderer::GetDevice().GetDevice();

        auto destroyPipeline = [&](VkPipeline& p, VkPipelineLayout& l)
        {
            if (p != VK_NULL_HANDLE) { vkDestroyPipeline(dev, p, nullptr); p = VK_NULL_HANDLE; }
            if (l != VK_NULL_HANDLE) { vkDestroyPipelineLayout(dev, l, nullptr); l = VK_NULL_HANDLE; }
        };
        destroyPipeline(m_ComputePipeline, m_ComputeLayout);
        destroyPipeline(m_RenderPipeline,  m_RenderLayout);

        if (m_ComputeDSL) { vkDestroyDescriptorSetLayout(dev, m_ComputeDSL, nullptr); m_ComputeDSL = VK_NULL_HANDLE; }
        if (m_RenderDSL)  { vkDestroyDescriptorSetLayout(dev, m_RenderDSL,  nullptr); m_RenderDSL  = VK_NULL_HANDLE; }

        m_ParticleBuffer.Destroy();
        m_IndirectBuffer.Destroy();
        m_ComputeUBO.Destroy();
    }

    void ParticleEmitter::CreateDescriptors()
    {
        VkDevice dev = Renderer::GetDevice().GetDevice();

        // Compute DSL: binding 0 = UBO, binding 1 = particle SSBO, binding 2 = indirect SSBO
        std::array<VkDescriptorSetLayoutBinding, 3> bindings{};
        bindings[0] = { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr };
        bindings[1] = { 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr };
        bindings[2] = { 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr };

        VkDescriptorSetLayoutCreateInfo dslInfo{};
        dslInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        dslInfo.bindingCount = static_cast<u32>(bindings.size());
        dslInfo.pBindings    = bindings.data();
        vkCreateDescriptorSetLayout(dev, &dslInfo, nullptr, &m_ComputeDSL);

        // Render DSL: binding 0 = texture sampler
        VkDescriptorSetLayoutBinding texBinding{ 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                  1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
        VkDescriptorSetLayoutCreateInfo rdslInfo{};
        rdslInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        rdslInfo.bindingCount = 1;
        rdslInfo.pBindings    = &texBinding;
        vkCreateDescriptorSetLayout(dev, &rdslInfo, nullptr, &m_RenderDSL);

        // Allocate compute set
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool     = Renderer::GetDescriptorPool();
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts        = &m_ComputeDSL;
        vkAllocateDescriptorSets(dev, &allocInfo, &m_ComputeDS);

        allocInfo.pSetLayouts = &m_RenderDSL;
        vkAllocateDescriptorSets(dev, &allocInfo, &m_RenderDS);

        // Write compute descriptors
        VkDescriptorBufferInfo uboInfo   = { m_ComputeUBO.GetBuffer(),     0, VK_WHOLE_SIZE };
        VkDescriptorBufferInfo ssboInfo  = { m_ParticleBuffer.GetBuffer(), 0, VK_WHOLE_SIZE };
        VkDescriptorBufferInfo indirInfo = { m_IndirectBuffer.GetBuffer(), 0, VK_WHOLE_SIZE };

        std::array<VkWriteDescriptorSet, 3> writes{};
        writes[0] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, m_ComputeDS, 0, 0, 1,
                      VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nullptr, &uboInfo, nullptr };
        writes[1] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, m_ComputeDS, 1, 0, 1,
                      VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &ssboInfo, nullptr };
        writes[2] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, m_ComputeDS, 2, 0, 1,
                      VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &indirInfo, nullptr };
        vkUpdateDescriptorSets(dev, static_cast<u32>(writes.size()), writes.data(), 0, nullptr);

        // Write render descriptor (texture)
        if (m_Desc.Texture)
        {
            VkDescriptorImageInfo imgInfo = m_Desc.Texture->GetDescriptorInfo();
            VkWriteDescriptorSet texWrite{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr,
                m_RenderDS, 0, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &imgInfo, nullptr, nullptr };
            vkUpdateDescriptorSets(dev, 1, &texWrite, 0, nullptr);
        }
    }

    void ParticleEmitter::CreateComputePipeline()
    {
        // Compute pipeline — loaded from pre-compiled SPIR-V
        // Shader loaded from Assets/Shaders/Particles.comp.spv
        // Full pipeline creation deferred to shader system in engine integration
        // (stub here to avoid hard dependency on shader path)
        FORCE_CORE_INFO("ParticleEmitter: compute pipeline deferred — load Particles.comp.spv");
    }

    void ParticleEmitter::CreateRenderPipeline()
    {
        FORCE_CORE_INFO("ParticleEmitter: render pipeline deferred — load Particles.vert/frag.spv");
    }

    void ParticleEmitter::Update(VkCommandBuffer cmd, f32 dt, f32 totalTime)
    {
        if (!m_Emitting || m_ComputePipeline == VK_NULL_HANDLE) return;

        // Fill compute UBO
        m_EmitAccum += dt;
        u32 emitCount = 0;
        if (m_Desc.EmissionRate > 0.0f)
        {
            f32 interval = 1.0f / m_Desc.EmissionRate;
            while (m_EmitAccum >= interval)
            {
                m_EmitAccum -= interval;
                ++emitCount;
            }
        }

        ParticleComputeUBO ubo{};
        ubo.EmitterPosition   = glm::vec4(m_Position, 0.0f);
        ubo.GravityDrag       = glm::vec4(m_Desc.Gravity, m_Desc.Drag);
        ubo.VelocityMin       = glm::vec4(m_Desc.InitialVelocityMin, 0);
        ubo.VelocityMax       = glm::vec4(m_Desc.InitialVelocityMax, 0);
        ubo.ColorStart        = m_Desc.ColorStart;
        ubo.ColorEnd          = m_Desc.ColorEnd;
        ubo.SizeStart         = m_Desc.SizeStart;
        ubo.SizeEnd           = m_Desc.SizeEnd;
        ubo.LifetimeMin       = m_Desc.LifetimeMin;
        ubo.LifetimeMax       = m_Desc.LifetimeMax;
        ubo.DeltaTime         = dt;
        ubo.TotalTime         = totalTime;
        ubo.EmitCount         = emitCount;
        ubo.MaxParticles      = m_Desc.MaxParticles;
        ubo.EmitterShapeType  = static_cast<u32>(m_Desc.Shape.Type);
        ubo.EmitterRadius     = m_Desc.Shape.Radius;
        ubo.EmitterAngle      = glm::radians(m_Desc.Shape.Angle);
        ubo.RandomSeed        = static_cast<u32>(totalTime * 1000.0f);
        m_ComputeUBO.SetData(&ubo, sizeof(ubo));

        // Dispatch compute
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_ComputePipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
                                 m_ComputeLayout, 0, 1, &m_ComputeDS, 0, nullptr);

        u32 groups = (m_Desc.MaxParticles + 63) / 64;
        vkCmdDispatch(cmd, groups, 1, 1);

        // Barrier: compute write → vertex read
        VkBufferMemoryBarrier barrier{};
        barrier.sType         = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
        barrier.buffer        = m_ParticleBuffer.GetBuffer();
        barrier.size          = VK_WHOLE_SIZE;
        vkCmdPipelineBarrier(cmd,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_VERTEX_INPUT_BIT | VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT,
            0, 0, nullptr, 1, &barrier, 0, nullptr);
    }

    void ParticleEmitter::Draw(VkCommandBuffer cmd)
    {
        if (m_RenderPipeline == VK_NULL_HANDLE) return;

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_RenderPipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                 m_RenderLayout, 0, 1, &m_RenderDS, 0, nullptr);

        VkBuffer buf     = m_ParticleBuffer.GetBuffer();
        VkDeviceSize off = 0;
        vkCmdBindVertexBuffers(cmd, 0, 1, &buf, &off);
        vkCmdDrawIndirect(cmd, m_IndirectBuffer.GetBuffer(), 0, 1, sizeof(VkDrawIndirectCommand));
    }

    // -------------------------------------------------------------------------
    // ParticleSystem

    ParticleSystem& ParticleSystem::Get()
    {
        static ParticleSystem instance;
        return instance;
    }

    void ParticleSystem::Init()
    {
        m_Initialized = true;
        FORCE_CORE_INFO("ParticleSystem initialized");
    }

    void ParticleSystem::Shutdown()
    {
        m_Emitters.clear();
        m_Initialized = false;
    }

    Ref<ParticleEmitter> ParticleSystem::CreateEmitter(const ParticleEmitterDesc& desc)
    {
        auto emitter = CreateRef<ParticleEmitter>();
        emitter->Create(desc);
        m_Emitters.push_back(emitter);
        return emitter;
    }

    void ParticleSystem::DestroyEmitter(const Ref<ParticleEmitter>& emitter)
    {
        m_Emitters.erase(
            std::remove(m_Emitters.begin(), m_Emitters.end(), emitter),
            m_Emitters.end());
    }

    void ParticleSystem::UpdateAll(VkCommandBuffer cmd, f32 dt, f32 totalTime)
    {
        for (auto& e : m_Emitters)
            e->Update(cmd, dt, totalTime);
    }

    void ParticleSystem::DrawAll(VkCommandBuffer cmd)
    {
        for (auto& e : m_Emitters)
            e->Draw(cmd);
    }
}
