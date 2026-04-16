#pragma once

#include "Force/Core/Base.h"
#include "Force/Renderer/Shader.h"
#include "Force/Renderer/VulkanBuffer.h"
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <vector>

namespace Force
{
    // Debug vertex: position + color
    struct DebugVertex
    {
        glm::vec3 Position;
        glm::vec4 Color;

        static VkVertexInputBindingDescription GetBindingDescription();
        static std::array<VkVertexInputAttributeDescription, 2> GetAttributeDescriptions();
    };

    // Immediate-mode debug renderer
    // Accumulates primitives during the frame, renders them all at once, then clears
    // Zero overhead in release builds when FORCE_DEBUG_RENDERER is not defined
    class DebugRenderer
    {
    public:
        static void Init();
        static void Shutdown();

        // Begin collecting debug primitives for this frame
        static void BeginFrame();

        // Render all accumulated primitives
        static void Render(VkCommandBuffer cmd);

        // End frame (clear buffers)
        static void EndFrame();

        // --- Drawing API ---
        // All calls between BeginFrame() and Render() accumulate geometry

        // Lines
        static void DrawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec4& color = glm::vec4(1.0f));

        // Boxes
        static void DrawBox(const glm::vec3& min, const glm::vec3& max, const glm::vec4& color = glm::vec4(1.0f));
        static void DrawBox(const glm::mat4& transform, const glm::vec3& halfExtents, const glm::vec4& color = glm::vec4(1.0f));

        // Spheres (wireframe)
        static void DrawSphere(const glm::vec3& center, float radius, const glm::vec4& color = glm::vec4(1.0f), u32 segments = 16);

        // Frustum (from inverse view-projection matrix)
        static void DrawFrustum(const glm::mat4& invViewProj, const glm::vec4& color = glm::vec4(1.0f));

        // Circle (in a plane)
        static void DrawCircle(const glm::vec3& center, const glm::vec3& normal, float radius, const glm::vec4& color = glm::vec4(1.0f), u32 segments = 32);

        // Axes (RGB = XYZ)
        static void DrawAxes(const glm::vec3& origin, float length = 1.0f);
        static void DrawAxes(const glm::mat4& transform, float length = 1.0f);

        // Grid
        static void DrawGrid(const glm::vec3& center, float size, u32 divisions, const glm::vec4& color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f));

        // Arrow (line + cone tip)
        static void DrawArrow(const glm::vec3& start, const glm::vec3& end, const glm::vec4& color = glm::vec4(1.0f), float tipLength = 0.1f);

    private:
        static void CreatePipeline();
        static void CreateBuffers();
        static void UploadVertices();

    private:
        static Ref<Shader>        s_Shader;
        static VkPipeline         s_Pipeline;
        static VkPipelineLayout   s_PipelineLayout;

        // Per-frame vertex data (CPU side)
        static std::vector<DebugVertex> s_Vertices;

        // GPU buffers (one per frame-in-flight)
        static std::vector<VulkanBuffer*> s_VertexBuffers;
        static u32 s_MaxVertices;

        static bool s_Initialized;
    };
}
