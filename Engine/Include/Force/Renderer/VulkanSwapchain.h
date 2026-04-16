#pragma once

#include "Force/Core/Base.h"
#include "Force/Renderer/VulkanDevice.h"
#include <vulkan/vulkan.h>
#include <vector>

namespace Force
{
    class VulkanSwapchain
    {
    public:
        VulkanSwapchain();
        ~VulkanSwapchain();
        
        void Init(u32 width, u32 height, bool vsync = true);
        void Shutdown();
        void Recreate(u32 width, u32 height);
        
        VkSwapchainKHR GetSwapchain() const { return m_Swapchain; }
        VkRenderPass GetRenderPass() const { return m_RenderPass; }
        VkFramebuffer GetFramebuffer(u32 index) const { return m_Framebuffers[index]; }
        VkExtent2D GetExtent() const { return m_Extent; }
        VkFormat GetImageFormat() const { return m_ImageFormat; }
        
        u32 GetImageCount() const { return static_cast<u32>(m_Images.size()); }
        u32 GetCurrentImageIndex() const { return m_CurrentImageIndex; }
        
        VkResult AcquireNextImage(VkSemaphore signalSemaphore);
        VkResult Present(VkSemaphore waitSemaphore);
        
    private:
        void CreateSwapchain(u32 width, u32 height, bool vsync);
        void CreateImageViews();
        void CreateRenderPass();
        void CreateDepthResources();
        void CreateFramebuffers();
        
        VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
        VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes, bool vsync);
        VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, u32 width, u32 height);
        
    private:
        VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
        VkFormat m_ImageFormat;
        VkExtent2D m_Extent;
        
        std::vector<VkImage> m_Images;
        std::vector<VkImageView> m_ImageViews;
        
        VkImage m_DepthImage = VK_NULL_HANDLE;
        VkImageView m_DepthImageView = VK_NULL_HANDLE;
        VmaAllocation m_DepthImageAllocation = VK_NULL_HANDLE;
        VkFormat m_DepthFormat;
        
        VkRenderPass m_RenderPass = VK_NULL_HANDLE;
        std::vector<VkFramebuffer> m_Framebuffers;
        
        u32 m_CurrentImageIndex = 0;
    };
}
