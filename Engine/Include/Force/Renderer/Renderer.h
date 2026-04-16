#pragma once

#include "Force/Core/Base.h"
#include "Force/Renderer/RendererTypes.h"
#include "Force/Renderer/VulkanContext.h"
#include "Force/Renderer/VulkanDevice.h"
#include "Force/Renderer/VulkanSwapchain.h"
#include <glm/glm.hpp>

namespace Force
{
    class Renderer
    {
    public:
        static void Init(const RendererConfig& config);
        static void Shutdown();
        
        static void BeginFrame();
        static void EndFrame();
        
        static void OnWindowResize(u32 width, u32 height);
        
        static void SetClearColor(const glm::vec4& color);
        
        static VulkanContext& GetContext() { return *s_Context; }
        static VulkanDevice& GetDevice() { return *s_Device; }
        static VulkanSwapchain& GetSwapchain() { return *s_Swapchain; }
        
        static u32 GetCurrentFrameIndex() { return s_CurrentFrame; }
        static u32 GetMaxFramesInFlight() { return s_MaxFramesInFlight; }
        
    private:
        static void CreateSyncObjects();
        static void DestroySyncObjects();
        static void CreateCommandBuffers();
        
    private:
        static Scope<VulkanContext> s_Context;
        static Scope<VulkanDevice> s_Device;
        static Scope<VulkanSwapchain> s_Swapchain;
        
        static std::vector<VkCommandBuffer> s_CommandBuffers;
        static std::vector<VkSemaphore> s_ImageAvailableSemaphores;
        static std::vector<VkSemaphore> s_RenderFinishedSemaphores;
        static std::vector<VkFence> s_InFlightFences;
        
        static u32 s_CurrentFrame;
        static u32 s_MaxFramesInFlight;
        static bool s_FramebufferResized;
        
        static glm::vec4 s_ClearColor;
    };
}
