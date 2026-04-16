#include "Force/Renderer/DebugRenderer.h"
#include "Force/Renderer/Renderer.h"
#include "Force/Renderer/VulkanDevice.h"
#include "Force/Core/Logger.h"
#include <glm/gtc/matrix_transform.hpp>

namespace Force
{
    // Static member definitions
    Ref<Shader>                   DebugRenderer::s_Shader;
    VkPipeline                    DebugRenderer::s_Pipeline       = VK_NULL_HANDLE;
    VkPipelineLayout              DebugRenderer::s_PipelineLayout = VK_NULL_HANDLE;
    std::vector<DebugVertex>      DebugRenderer::s_Vertices;
    std::vector<VulkanBuffer*>    DebugRenderer::s_VertexBuffers;
    u32                           DebugRenderer::s_MaxVertices    = 100000;
    bool                          DebugRenderer::s_Initialized    = false;

    // -------------------------------------------------------------------------
    // DebugVertex
    // -------------------------------------------------------------------------
    VkVertexInputBindingDescription DebugVertex::GetBindingDescription()
    {
        VkVertexInputBindingDescription binding{};
        binding.binding   = 0;
        binding.stride    = sizeof(DebugVertex);
        binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return binding;
    }

    std::array<VkVertexInputAttributeDescription, 2> DebugVertex::GetAttributeDescriptions()
    {
        std::array<VkVertexInputAttributeDescription, 2> attrs{};

        // Position
        attrs[0].binding  = 0;
        attrs[0].location = 0;
        attrs[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
        attrs[0].offset   = offsetof(DebugVertex, Position);

        // Color
        attrs[1].binding  = 0;
        attrs[1].location = 1;
        attrs[1].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
        attrs[1].offset   = offsetof(DebugVertex, Color);

        return attrs;
    }

    // -------------------------------------------------------------------------
    // DebugRenderer
    // -------------------------------------------------------------------------
    void DebugRenderer::Init()
    {
        if (s_Initialized)
            return;

        CreateBuffers();
        CreatePipeline();

        s_Initialized = true;
        FORCE_CORE_INFO("DebugRenderer initialized");
    }

    void DebugRenderer::Shutdown()
    {
        if (!s_Initialized)
            return;

        VkDevice device = VulkanDevice::Get().GetDevice();
        vkDeviceWaitIdle(device);

        // Destroy vertex buffers
        for (auto* buffer : s_VertexBuffers)
        {
            if (buffer)
            {
                buffer->Destroy();
                delete buffer;
            }
        }
        s_VertexBuffers.clear();

        // Destroy pipeline
        if (s_Pipeline != VK_NULL_HANDLE)
        {
            vkDestroyPipeline(device, s_Pipeline, nullptr);
            s_Pipeline = VK_NULL_HANDLE;
        }

        // Destroy pipeline layout
        if (s_PipelineLayout != VK_NULL_HANDLE)
        {
            vkDestroyPipelineLayout(device, s_PipelineLayout, nullptr);
            s_PipelineLayout = VK_NULL_HANDLE;
        }

        s_Shader.reset();
        s_Vertices.clear();
        s_Initialized = false;
    }

    void DebugRenderer::CreateBuffers()
    {
        u32 framesInFlight = Renderer::GetMaxFramesInFlight();
        s_VertexBuffers.resize(framesInFlight);

        for (u32 i = 0; i < framesInFlight; ++i)
        {
            s_VertexBuffers[i] = new VulkanBuffer();
            s_VertexBuffers[i]->Create(
                s_MaxVertices * sizeof(DebugVertex),
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VMA_MEMORY_USAGE_CPU_TO_GPU
            );
        }

        s_Vertices.reserve(s_MaxVertices);
    }

    void DebugRenderer::CreatePipeline()
    {
        VkDevice device = VulkanDevice::Get().GetDevice();

        // Create pipeline layout with camera descriptor set (set 0) and push constants
        VkDescriptorSetLayout cameraLayout = Renderer::GetCameraDescriptorSetLayout();

        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.setLayoutCount = 1;
        layoutInfo.pSetLayouts    = &cameraLayout;

        VkResult result = vkCreatePipelineLayout(device, &layoutInfo, nullptr, &s_PipelineLayout);
        FORCE_CORE_ASSERT(result == VK_SUCCESS, "Failed to create debug pipeline layout");

        // Pipeline will be created when we have shaders compiled
        // For now, s_Pipeline remains VK_NULL_HANDLE and Render() will skip drawing
    }

    void DebugRenderer::BeginFrame()
    {
        s_Vertices.clear();
    }

    void DebugRenderer::UploadVertices()
    {
        if (s_Vertices.empty())
            return;

        u32 currentFrame = Renderer::GetCurrentFrameIndex();
        u32 dataSize = static_cast<u32>(s_Vertices.size() * sizeof(DebugVertex));

        // Clamp to max buffer size
        if (s_Vertices.size() > s_MaxVertices)
        {
            FORCE_CORE_WARN("DebugRenderer: vertex limit exceeded ({} > {}), truncating", s_Vertices.size(), s_MaxVertices);
            dataSize = s_MaxVertices * sizeof(DebugVertex);
        }

        s_VertexBuffers[currentFrame]->SetData(s_Vertices.data(), dataSize);
    }

    void DebugRenderer::Render(VkCommandBuffer cmd)
    {
        if (s_Vertices.empty())
            return;

        // Pipeline not yet created (shaders not compiled)
        if (s_Pipeline == VK_NULL_HANDLE)
        {
            // TODO: Create pipeline here if shader files exist
            return;
        }

        UploadVertices();

        u32 currentFrame = Renderer::GetCurrentFrameIndex();
        u32 vertexCount = static_cast<u32>(std::min(s_Vertices.size(), static_cast<size_t>(s_MaxVertices)));

        // Bind pipeline
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, s_Pipeline);

        // Bind camera descriptor set
        VkDescriptorSet cameraSet = Renderer::GetCameraDescriptorSet();
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                 s_PipelineLayout, 0, 1, &cameraSet, 0, nullptr);

        // Bind vertex buffer
        VkBuffer vertexBuffers[] = { s_VertexBuffers[currentFrame]->GetBuffer() };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);

        // Draw
        vkCmdDraw(cmd, vertexCount, 1, 0, 0);
    }

    void DebugRenderer::EndFrame()
    {
        s_Vertices.clear();
    }

    // -------------------------------------------------------------------------
    // Drawing API
    // -------------------------------------------------------------------------
    void DebugRenderer::DrawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec4& color)
    {
        s_Vertices.push_back({ start, color });
        s_Vertices.push_back({ end, color });
    }

    void DebugRenderer::DrawBox(const glm::vec3& min, const glm::vec3& max, const glm::vec4& color)
    {
        glm::vec3 corners[8] = {
            { min.x, min.y, min.z },
            { max.x, min.y, min.z },
            { max.x, max.y, min.z },
            { min.x, max.y, min.z },
            { min.x, min.y, max.z },
            { max.x, min.y, max.z },
            { max.x, max.y, max.z },
            { min.x, max.y, max.z }
        };

        // Bottom face
        DrawLine(corners[0], corners[1], color);
        DrawLine(corners[1], corners[2], color);
        DrawLine(corners[2], corners[3], color);
        DrawLine(corners[3], corners[0], color);

        // Top face
        DrawLine(corners[4], corners[5], color);
        DrawLine(corners[5], corners[6], color);
        DrawLine(corners[6], corners[7], color);
        DrawLine(corners[7], corners[4], color);

        // Vertical edges
        DrawLine(corners[0], corners[4], color);
        DrawLine(corners[1], corners[5], color);
        DrawLine(corners[2], corners[6], color);
        DrawLine(corners[3], corners[7], color);
    }

    void DebugRenderer::DrawBox(const glm::mat4& transform, const glm::vec3& halfExtents, const glm::vec4& color)
    {
        glm::vec3 corners[8] = {
            glm::vec3(transform * glm::vec4(-halfExtents.x, -halfExtents.y, -halfExtents.z, 1.0f)),
            glm::vec3(transform * glm::vec4( halfExtents.x, -halfExtents.y, -halfExtents.z, 1.0f)),
            glm::vec3(transform * glm::vec4( halfExtents.x,  halfExtents.y, -halfExtents.z, 1.0f)),
            glm::vec3(transform * glm::vec4(-halfExtents.x,  halfExtents.y, -halfExtents.z, 1.0f)),
            glm::vec3(transform * glm::vec4(-halfExtents.x, -halfExtents.y,  halfExtents.z, 1.0f)),
            glm::vec3(transform * glm::vec4( halfExtents.x, -halfExtents.y,  halfExtents.z, 1.0f)),
            glm::vec3(transform * glm::vec4( halfExtents.x,  halfExtents.y,  halfExtents.z, 1.0f)),
            glm::vec3(transform * glm::vec4(-halfExtents.x,  halfExtents.y,  halfExtents.z, 1.0f))
        };

        // Bottom face
        DrawLine(corners[0], corners[1], color);
        DrawLine(corners[1], corners[2], color);
        DrawLine(corners[2], corners[3], color);
        DrawLine(corners[3], corners[0], color);

        // Top face
        DrawLine(corners[4], corners[5], color);
        DrawLine(corners[5], corners[6], color);
        DrawLine(corners[6], corners[7], color);
        DrawLine(corners[7], corners[4], color);

        // Vertical edges
        DrawLine(corners[0], corners[4], color);
        DrawLine(corners[1], corners[5], color);
        DrawLine(corners[2], corners[6], color);
        DrawLine(corners[3], corners[7], color);
    }

    void DebugRenderer::DrawSphere(const glm::vec3& center, float radius, const glm::vec4& color, u32 segments)
    {
        // Draw 3 orthogonal circles
        DrawCircle(center, glm::vec3(1.0f, 0.0f, 0.0f), radius, color, segments); // YZ plane
        DrawCircle(center, glm::vec3(0.0f, 1.0f, 0.0f), radius, color, segments); // XZ plane
        DrawCircle(center, glm::vec3(0.0f, 0.0f, 1.0f), radius, color, segments); // XY plane
    }

    void DebugRenderer::DrawFrustum(const glm::mat4& invViewProj, const glm::vec4& color)
    {
        // NDC corners
        glm::vec4 ndcCorners[8] = {
            { -1, -1, 0, 1 }, // near bottom-left
            {  1, -1, 0, 1 }, // near bottom-right
            {  1,  1, 0, 1 }, // near top-right
            { -1,  1, 0, 1 }, // near top-left
            { -1, -1, 1, 1 }, // far bottom-left
            {  1, -1, 1, 1 }, // far bottom-right
            {  1,  1, 1, 1 }, // far top-right
            { -1,  1, 1, 1 }  // far top-left
        };

        glm::vec3 worldCorners[8];
        for (int i = 0; i < 8; ++i)
        {
            glm::vec4 world = invViewProj * ndcCorners[i];
            worldCorners[i] = glm::vec3(world) / world.w;
        }

        // Near plane
        DrawLine(worldCorners[0], worldCorners[1], color);
        DrawLine(worldCorners[1], worldCorners[2], color);
        DrawLine(worldCorners[2], worldCorners[3], color);
        DrawLine(worldCorners[3], worldCorners[0], color);

        // Far plane
        DrawLine(worldCorners[4], worldCorners[5], color);
        DrawLine(worldCorners[5], worldCorners[6], color);
        DrawLine(worldCorners[6], worldCorners[7], color);
        DrawLine(worldCorners[7], worldCorners[4], color);

        // Connecting edges
        DrawLine(worldCorners[0], worldCorners[4], color);
        DrawLine(worldCorners[1], worldCorners[5], color);
        DrawLine(worldCorners[2], worldCorners[6], color);
        DrawLine(worldCorners[3], worldCorners[7], color);
    }

    void DebugRenderer::DrawCircle(const glm::vec3& center, const glm::vec3& normal, float radius, const glm::vec4& color, u32 segments)
    {
        // Build a coordinate frame with normal as one axis
        glm::vec3 up = glm::abs(normal.y) < 0.999f ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
        glm::vec3 tangent = glm::normalize(glm::cross(up, normal));
        glm::vec3 bitangent = glm::cross(normal, tangent);

        float angleStep = glm::two_pi<float>() / static_cast<float>(segments);
        glm::vec3 prevPoint = center + tangent * radius;

        for (u32 i = 1; i <= segments; ++i)
        {
            float angle = angleStep * static_cast<float>(i);
            glm::vec3 point = center + (tangent * glm::cos(angle) + bitangent * glm::sin(angle)) * radius;
            DrawLine(prevPoint, point, color);
            prevPoint = point;
        }
    }

    void DebugRenderer::DrawAxes(const glm::vec3& origin, float length)
    {
        DrawLine(origin, origin + glm::vec3(length, 0, 0), glm::vec4(1, 0, 0, 1)); // X = Red
        DrawLine(origin, origin + glm::vec3(0, length, 0), glm::vec4(0, 1, 0, 1)); // Y = Green
        DrawLine(origin, origin + glm::vec3(0, 0, length), glm::vec4(0, 0, 1, 1)); // Z = Blue
    }

    void DebugRenderer::DrawAxes(const glm::mat4& transform, float length)
    {
        glm::vec3 origin = glm::vec3(transform[3]);
        glm::vec3 xAxis = glm::vec3(transform[0]) * length;
        glm::vec3 yAxis = glm::vec3(transform[1]) * length;
        glm::vec3 zAxis = glm::vec3(transform[2]) * length;

        DrawLine(origin, origin + xAxis, glm::vec4(1, 0, 0, 1));
        DrawLine(origin, origin + yAxis, glm::vec4(0, 1, 0, 1));
        DrawLine(origin, origin + zAxis, glm::vec4(0, 0, 1, 1));
    }

    void DebugRenderer::DrawGrid(const glm::vec3& center, float size, u32 divisions, const glm::vec4& color)
    {
        float halfSize = size * 0.5f;
        float step = size / static_cast<float>(divisions);

        for (u32 i = 0; i <= divisions; ++i)
        {
            float offset = -halfSize + step * static_cast<float>(i);

            // Lines along X
            DrawLine(
                glm::vec3(center.x - halfSize, center.y, center.z + offset),
                glm::vec3(center.x + halfSize, center.y, center.z + offset),
                color
            );

            // Lines along Z
            DrawLine(
                glm::vec3(center.x + offset, center.y, center.z - halfSize),
                glm::vec3(center.x + offset, center.y, center.z + halfSize),
                color
            );
        }
    }

    void DebugRenderer::DrawArrow(const glm::vec3& start, const glm::vec3& end, const glm::vec4& color, float tipLength)
    {
        DrawLine(start, end, color);

        glm::vec3 dir = end - start;
        float length = glm::length(dir);
        if (length < 0.001f)
            return;

        dir /= length;
        float actualTipLength = length * tipLength;

        // Build perpendicular vectors for the arrow head
        glm::vec3 up = glm::abs(dir.y) < 0.999f ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
        glm::vec3 right = glm::normalize(glm::cross(up, dir));
        glm::vec3 upDir = glm::cross(dir, right);

        glm::vec3 tipBase = end - dir * actualTipLength;
        float tipWidth = actualTipLength * 0.5f;

        // 4 lines forming the arrow head
        DrawLine(end, tipBase + right * tipWidth, color);
        DrawLine(end, tipBase - right * tipWidth, color);
        DrawLine(end, tipBase + upDir * tipWidth, color);
        DrawLine(end, tipBase - upDir * tipWidth, color);
    }
}
