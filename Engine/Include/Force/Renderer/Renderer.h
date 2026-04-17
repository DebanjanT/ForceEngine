#pragma once

#include "Force/Core/Base.h"
#include "Force/Renderer/RendererTypes.h"
#include "Force/Renderer/VulkanContext.h"
#include "Force/Renderer/VulkanDevice.h"
#include "Force/Renderer/VulkanSwapchain.h"
#include "Force/Renderer/VulkanBuffer.h"
#include <glm/glm.hpp>

namespace Force
{
    // Camera data uploaded to GPU every frame (set 0, binding 0)
    struct CameraUBO
    {
        glm::mat4 view;
        glm::mat4 proj;
        glm::vec4 camPos; // xyz = position, w unused
    };

    class Renderer
    {
    public:
        static void Init(const RendererConfig& config);
        static void Shutdown();

        static void BeginFrame();
        static void EndFrame();

        static void OnWindowResize(u32 width, u32 height);

        static void SetClearColor(const glm::vec4& color);

        // Update camera data for the current frame
        static void UpdateCamera(const CameraUBO& data);

        // Accessors
        static VulkanContext&  GetContext()   { return *s_Context; }
        static VulkanDevice&   GetDevice()    { return *s_Device; }
        static VulkanSwapchain& GetSwapchain() { return *s_Swapchain; }

        static u32 GetCurrentFrameIndex()  { return s_CurrentFrame; }
        static u32 GetMaxFramesInFlight()  { return s_MaxFramesInFlight; }

        static VkCommandBuffer GetCurrentCommandBuffer() { return s_CommandBuffers[s_CurrentFrame]; }

        // Global descriptor pool — used by Material and other systems
        static VkDescriptorPool GetDescriptorPool() { return s_DescriptorPool; }

        // Per-frame camera descriptor set (set 0)
        static VkDescriptorSet        GetCameraDescriptorSet()       { return s_CameraDescriptorSets[s_CurrentFrame]; }
        static VkDescriptorSetLayout  GetCameraDescriptorSetLayout() { return s_CameraDescriptorSetLayout; }

    private:
        static void CreateSyncObjects();
        static void DestroySyncObjects();
        static void CreateCommandBuffers();
        static void CreateDescriptorPool();
        static void CreateCameraResources();
        static void InitImGui();
        static void ShutdownImGui();
        static void RenderImGui(VkCommandBuffer cmd);

    private:
        static Scope<VulkanContext>   s_Context;
        static Scope<VulkanDevice>    s_Device;
        static Scope<VulkanSwapchain> s_Swapchain;

        static std::vector<VkCommandBuffer> s_CommandBuffers;
        static std::vector<VkSemaphore>     s_ImageAvailableSemaphores;
        static std::vector<VkSemaphore>     s_RenderFinishedSemaphores;
        static std::vector<VkFence>         s_InFlightFences;

        static u32  s_CurrentFrame;
        static u32  s_MaxFramesInFlight;
        static bool s_FramebufferResized;
        static bool s_FrameStarted;

        static glm::vec4 s_ClearColor;

        // Global descriptor pool
        static VkDescriptorPool s_DescriptorPool;

        // Camera (set 0) resources — one per frame-in-flight
        static VkDescriptorSetLayout        s_CameraDescriptorSetLayout;
        static std::vector<UniformBuffer>   s_CameraUBOs;
        static std::vector<VkDescriptorSet> s_CameraDescriptorSets;
    };
}
