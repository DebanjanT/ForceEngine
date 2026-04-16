#pragma once

#include "Force/Core/Base.h"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <vector>
#include <optional>

namespace Force
{
    struct QueueFamilyIndices
    {
        std::optional<u32> GraphicsFamily;
        std::optional<u32> PresentFamily;
        std::optional<u32> ComputeFamily;
        std::optional<u32> TransferFamily;
        
        bool IsComplete() const
        {
            return GraphicsFamily.has_value() && PresentFamily.has_value();
        }
    };
    
    struct SwapchainSupportDetails
    {
        VkSurfaceCapabilitiesKHR Capabilities;
        std::vector<VkSurfaceFormatKHR> Formats;
        std::vector<VkPresentModeKHR> PresentModes;
    };
    
    class VulkanDevice
    {
    public:
        VulkanDevice();
        ~VulkanDevice();
        
        void Init(VkInstance instance, VkSurfaceKHR surface);
        void Shutdown();
        
        VkDevice GetDevice() const { return m_Device; }
        VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
        VmaAllocator GetAllocator() const { return m_Allocator; }
        
        VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }
        VkQueue GetPresentQueue() const { return m_PresentQueue; }
        VkQueue GetComputeQueue() const { return m_ComputeQueue; }
        
        const QueueFamilyIndices& GetQueueFamilies() const { return m_QueueFamilies; }
        SwapchainSupportDetails QuerySwapchainSupport() const;
        
        VkCommandPool GetCommandPool() const { return m_CommandPool; }
        
        u32 FindMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties) const;
        VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, 
                                     VkImageTiling tiling, 
                                     VkFormatFeatureFlags features) const;
        
        void WaitIdle() const;
        
        static VulkanDevice& Get() { return *s_Instance; }
        
    private:
        void PickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface);
        void CreateLogicalDevice(VkSurfaceKHR surface);
        void CreateAllocator(VkInstance instance);
        void CreateCommandPool();
        
        bool IsDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface);
        bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
        QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
        i32 RateDeviceSuitability(VkPhysicalDevice device);
        
    private:
        VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
        VkDevice m_Device = VK_NULL_HANDLE;
        VmaAllocator m_Allocator = VK_NULL_HANDLE;
        
        VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
        VkQueue m_PresentQueue = VK_NULL_HANDLE;
        VkQueue m_ComputeQueue = VK_NULL_HANDLE;
        
        VkCommandPool m_CommandPool = VK_NULL_HANDLE;
        
        QueueFamilyIndices m_QueueFamilies;
        VkPhysicalDeviceProperties m_DeviceProperties;
        VkPhysicalDeviceFeatures m_DeviceFeatures;
        
        static VulkanDevice* s_Instance;
        
        const std::vector<const char*> m_DeviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };
    };
}
