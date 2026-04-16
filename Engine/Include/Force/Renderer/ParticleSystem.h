#pragma once

#include "Force/Core/Base.h"
#include "Force/Renderer/Texture.h"
#include "Force/Renderer/VulkanBuffer.h"
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <vector>
#include "Mesh.h"
#include <functional>

namespace Force
{
    // -------------------------------------------------------------------------
    // Per-particle data (GPU layout — matches compute shader struct)
    struct alignas(16) ParticleGPU
    {
        glm::vec4 Position;      // xyz = position, w = age [0,1]
        glm::vec4 Velocity;      // xyz = velocity, w = lifetime (seconds)
        glm::vec4 Color;         // rgba
        glm::vec2 Size;          // width, height
        f32       Rotation;      // billboard rotation radians
        f32       _pad;
    };

    // -------------------------------------------------------------------------
    // Emitter shape
    enum class EmitterShape { Point, Sphere, Cone, Box, MeshSurface };

    struct EmitterShapeDesc
    {
        EmitterShape Type    = EmitterShape::Point;
        f32          Radius  = 1.0f;      // Sphere / Cone base
        f32          Angle   = 25.0f;     // Cone half-angle (degrees)
        glm::vec3    Extents = { 1,1,1 }; // Box
        Ref<Mesh>    Surface;             // MeshSurface
    };

    // -------------------------------------------------------------------------
    // CPU-side emitter description
    struct ParticleEmitterDesc
    {
        EmitterShapeDesc  Shape;
        u32               MaxParticles      = 1000;
        f32               EmissionRate      = 100.0f;  // particles / second
        f32               LifetimeMin       = 1.0f;
        f32               LifetimeMax       = 2.0f;
        glm::vec3         InitialVelocityMin = { -1, 2, -1 };
        glm::vec3         InitialVelocityMax = { 1, 5, 1 };
        glm::vec4         ColorStart        = { 1, 1, 1, 1 };
        glm::vec4         ColorEnd          = { 1, 1, 1, 0 };
        glm::vec2         SizeStart         = { 0.1f, 0.1f };
        glm::vec2         SizeEnd           = { 0.0f, 0.0f };
        f32               Drag              = 0.1f;
        glm::vec3         Gravity           = { 0, -9.81f, 0 };
        bool              WorldSpace        = true;
        bool              CollisionEnabled  = false;
        Ref<Texture2D>    Texture;
    };

    // -------------------------------------------------------------------------
    // Uniform block for compute shader
    struct alignas(16) ParticleComputeUBO
    {
        glm::vec4  EmitterPosition;
        glm::vec4  GravityDrag;          // xyz = gravity, w = drag
        glm::vec4  VelocityMin;
        glm::vec4  VelocityMax;
        glm::vec4  ColorStart;
        glm::vec4  ColorEnd;
        glm::vec2  SizeStart;
        glm::vec2  SizeEnd;
        f32        LifetimeMin;
        f32        LifetimeMax;
        f32        DeltaTime;
        f32        TotalTime;
        u32        EmitCount;
        u32        MaxParticles;
        u32        EmitterShapeType;
        f32        EmitterRadius;
        f32        EmitterAngle;
        u32        RandomSeed;
        u32        _pad[2];
    };

    // -------------------------------------------------------------------------
    class ParticleEmitter
    {
    public:
        ParticleEmitter() = default;
        ~ParticleEmitter();

        void Create(const ParticleEmitterDesc& desc);
        void Destroy();

        void SetPosition(const glm::vec3& pos) { m_Position = pos; }
        void SetEmitting(bool emit)             { m_Emitting = emit; }
        bool IsEmitting() const                 { return m_Emitting; }

        // Called by ParticleSystem — submits compute dispatch
        void Update(VkCommandBuffer cmd, f32 dt, f32 totalTime);

        // Called by ParticleSystem — issues draw
        void Draw(VkCommandBuffer cmd);

        u32 GetMaxParticles() const { return m_Desc.MaxParticles; }

    private:
        void CreateComputePipeline();
        void CreateRenderPipeline();
        void CreateDescriptors();

    private:
        ParticleEmitterDesc m_Desc;
        glm::vec3           m_Position   = glm::vec3(0.0f);
        bool                m_Emitting   = true;
        f32                 m_EmitAccum  = 0.0f;
        u32                 m_AliveCount = 0;

        // GPU particle buffer (ping-pong)
        VulkanBuffer m_ParticleBuffer;
        VulkanBuffer m_IndirectBuffer;    // VkDrawIndirectCommand
        VulkanBuffer m_ComputeUBO;

        VkPipeline        m_ComputePipeline = VK_NULL_HANDLE;
        VkPipelineLayout  m_ComputeLayout   = VK_NULL_HANDLE;
        VkPipeline        m_RenderPipeline  = VK_NULL_HANDLE;
        VkPipelineLayout  m_RenderLayout    = VK_NULL_HANDLE;

        VkDescriptorSetLayout m_ComputeDSL  = VK_NULL_HANDLE;
        VkDescriptorSet       m_ComputeDS   = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_RenderDSL   = VK_NULL_HANDLE;
        VkDescriptorSet       m_RenderDS    = VK_NULL_HANDLE;
    };

    // -------------------------------------------------------------------------
    class ParticleSystem
    {
    public:
        static ParticleSystem& Get();

        void Init();
        void Shutdown();

        Ref<ParticleEmitter> CreateEmitter(const ParticleEmitterDesc& desc);
        void                 DestroyEmitter(const Ref<ParticleEmitter>& emitter);

        // Dispatch all compute updates (before render pass)
        void UpdateAll(VkCommandBuffer cmd, f32 dt, f32 totalTime);

        // Draw all emitters (inside render pass)
        void DrawAll(VkCommandBuffer cmd);

    private:
        std::vector<Ref<ParticleEmitter>> m_Emitters;
        bool m_Initialized = false;
    };
}
