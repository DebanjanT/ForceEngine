#include "Force/Renderer/VulkanDevice.h"
#include "Force/Renderer/VulkanContext.h"
#include "Force/Core/Logger.h"
#include <set>
#include <map>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

namespace Force
{
    VulkanDevice* VulkanDevice::s_Instance = nullptr;
    
    VulkanDevice::VulkanDevice()
    {
        FORCE_CORE_ASSERT(!s_Instance, "VulkanDevice already exists!");
        s_Instance = this;
    }
    
    VulkanDevice::~VulkanDevice()
    {
        s_Instance = nullptr;
    }
    
    void VulkanDevice::Init(VkInstance instance, VkSurfaceKHR surface)
    {
        FORCE_CORE_INFO("Initializing Vulkan device...");
        
        PickPhysicalDevice(instance, surface);
        CreateLogicalDevice(surface);
        CreateAllocator(instance);
        CreateCommandPool();
        
        FORCE_CORE_INFO("Vulkan device initialized: {}", m_DeviceProperties.deviceName);
    }
    
    void VulkanDevice::Shutdown()
    {
        FORCE_CORE_INFO("Shutting down Vulkan device...");
        
        WaitIdle();
        
        if (m_CommandPool != VK_NULL_HANDLE)
        {
            vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
        }
        
        if (m_Allocator != VK_NULL_HANDLE)
        {
            vmaDestroyAllocator(m_Allocator);
        }
        
        if (m_Device != VK_NULL_HANDLE)
        {
            vkDestroyDevice(m_Device, nullptr);
        }
    }
    
    void VulkanDevice::PickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface)
    {
        u32 deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        
        FORCE_CORE_ASSERT(deviceCount > 0, "Failed to find GPUs with Vulkan support!");
        
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
        
        std::multimap<i32, VkPhysicalDevice> candidates;
        
        for (const auto& device : devices)
        {
            if (IsDeviceSuitable(device, surface))
            {
                i32 score = RateDeviceSuitability(device);
                candidates.insert(std::make_pair(score, device));
            }
        }
        
        if (!candidates.empty() && candidates.rbegin()->first > 0)
        {
            m_PhysicalDevice = candidates.rbegin()->second;
            vkGetPhysicalDeviceProperties(m_PhysicalDevice, &m_DeviceProperties);
            vkGetPhysicalDeviceFeatures(m_PhysicalDevice, &m_DeviceFeatures);
            m_QueueFamilies = FindQueueFamilies(m_PhysicalDevice, surface);
        }
        else
        {
            FORCE_CORE_ASSERT(false, "Failed to find a suitable GPU!");
        }
    }
    
    void VulkanDevice::CreateLogicalDevice(VkSurfaceKHR surface)
    {
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<u32> uniqueQueueFamilies = {
            m_QueueFamilies.GraphicsFamily.value(),
            m_QueueFamilies.PresentFamily.value()
        };
        
        float queuePriority = 1.0f;
        for (u32 queueFamily : uniqueQueueFamilies)
        {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }
        
        VkPhysicalDeviceFeatures deviceFeatures{};
        deviceFeatures.samplerAnisotropy = VK_TRUE;
        deviceFeatures.fillModeNonSolid = VK_TRUE;
        deviceFeatures.wideLines = VK_TRUE;

        // Enable Vulkan 1.3 core features: dynamic rendering + synchronization2
        VkPhysicalDeviceVulkan13Features features13{};
        features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
        features13.dynamicRendering = VK_TRUE;
        features13.synchronization2 = VK_TRUE;

        VkPhysicalDeviceFeatures2 features2{};
        features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        features2.features = deviceFeatures;
        features2.pNext = &features13;

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = static_cast<u32>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.pEnabledFeatures = nullptr; // Must be null when using pNext features chain
        createInfo.pNext = &features2;
        createInfo.enabledExtensionCount = static_cast<u32>(m_DeviceExtensions.size());
        createInfo.ppEnabledExtensionNames = m_DeviceExtensions.data();
        
        VkResult result = vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_Device);
        FORCE_CORE_ASSERT(result == VK_SUCCESS, "Failed to create logical device!");
        
        vkGetDeviceQueue(m_Device, m_QueueFamilies.GraphicsFamily.value(), 0, &m_GraphicsQueue);
        vkGetDeviceQueue(m_Device, m_QueueFamilies.PresentFamily.value(), 0, &m_PresentQueue);
        
        if (m_QueueFamilies.ComputeFamily.has_value())
        {
            vkGetDeviceQueue(m_Device, m_QueueFamilies.ComputeFamily.value(), 0, &m_ComputeQueue);
        }
    }
    
    void VulkanDevice::CreateAllocator(VkInstance instance)
    {
        VmaAllocatorCreateInfo allocatorInfo{};
        allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3;
        allocatorInfo.physicalDevice = m_PhysicalDevice;
        allocatorInfo.device = m_Device;
        allocatorInfo.instance = instance;
        
        VkResult result = vmaCreateAllocator(&allocatorInfo, &m_Allocator);
        FORCE_CORE_ASSERT(result == VK_SUCCESS, "Failed to create VMA allocator!");
    }
    
    void VulkanDevice::CreateCommandPool()
    {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = m_QueueFamilies.GraphicsFamily.value();
        
        VkResult result = vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_CommandPool);
        FORCE_CORE_ASSERT(result == VK_SUCCESS, "Failed to create command pool!");
    }
    
    bool VulkanDevice::IsDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface)
    {
        QueueFamilyIndices indices = FindQueueFamilies(device, surface);
        bool extensionsSupported = CheckDeviceExtensionSupport(device);
        
        bool swapchainAdequate = false;
        if (extensionsSupported)
        {
            // Query swapchain support for this specific device/surface
            SwapchainSupportDetails swapchainSupport;
            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &swapchainSupport.Capabilities);
            
            u32 formatCount;
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
            if (formatCount != 0)
            {
                swapchainSupport.Formats.resize(formatCount);
                vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, swapchainSupport.Formats.data());
            }
            
            u32 presentModeCount;
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
            if (presentModeCount != 0)
            {
                swapchainSupport.PresentModes.resize(presentModeCount);
                vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, swapchainSupport.PresentModes.data());
            }
            
            swapchainAdequate = !swapchainSupport.Formats.empty() && !swapchainSupport.PresentModes.empty();
        }
        
        VkPhysicalDeviceFeatures supportedFeatures;
        vkGetPhysicalDeviceFeatures(device, &supportedFeatures);
        
        return indices.IsComplete() && extensionsSupported && swapchainAdequate && supportedFeatures.samplerAnisotropy;
    }
    
    bool VulkanDevice::CheckDeviceExtensionSupport(VkPhysicalDevice device)
    {
        u32 extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
        
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
        
        std::set<std::string> requiredExtensions(m_DeviceExtensions.begin(), m_DeviceExtensions.end());
        
        for (const auto& extension : availableExtensions)
        {
            requiredExtensions.erase(extension.extensionName);
        }
        
        return requiredExtensions.empty();
    }
    
    QueueFamilyIndices VulkanDevice::FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface)
    {
        QueueFamilyIndices indices;
        
        u32 queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
        
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
        
        u32 i = 0;
        for (const auto& queueFamily : queueFamilies)
        {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                indices.GraphicsFamily = i;
            }
            
            if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)
            {
                indices.ComputeFamily = i;
            }
            
            if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT)
            {
                indices.TransferFamily = i;
            }
            
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
            
            if (presentSupport)
            {
                indices.PresentFamily = i;
            }
            
            if (indices.IsComplete())
            {
                break;
            }
            
            i++;
        }
        
        return indices;
    }
    
    i32 VulkanDevice::RateDeviceSuitability(VkPhysicalDevice device)
    {
        VkPhysicalDeviceProperties deviceProperties;
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
        
        i32 score = 0;
        
        if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            score += 1000;
        }
        
        score += deviceProperties.limits.maxImageDimension2D;
        
        if (!deviceFeatures.geometryShader)
        {
            return 0;
        }
        
        return score;
    }
    
    SwapchainSupportDetails VulkanDevice::QuerySwapchainSupport() const
    {
        SwapchainSupportDetails details;
        VkSurfaceKHR surface = VulkanContext::Get().GetSurface();
        
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice, surface, &details.Capabilities);
        
        u32 formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, surface, &formatCount, nullptr);
        
        if (formatCount != 0)
        {
            details.Formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, surface, &formatCount, details.Formats.data());
        }
        
        u32 presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, surface, &presentModeCount, nullptr);
        
        if (presentModeCount != 0)
        {
            details.PresentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, surface, &presentModeCount, details.PresentModes.data());
        }
        
        return details;
    }
    
    u32 VulkanDevice::FindMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties) const
    {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memProperties);
        
        for (u32 i = 0; i < memProperties.memoryTypeCount; i++)
        {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }
        
        FORCE_CORE_ASSERT(false, "Failed to find suitable memory type!");
        return 0;
    }
    
    VkFormat VulkanDevice::FindSupportedFormat(const std::vector<VkFormat>& candidates, 
                                               VkImageTiling tiling, 
                                               VkFormatFeatureFlags features) const
    {
        for (VkFormat format : candidates)
        {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, format, &props);
            
            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
            {
                return format;
            }
            else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
            {
                return format;
            }
        }
        
        FORCE_CORE_ASSERT(false, "Failed to find supported format!");
        return VK_FORMAT_UNDEFINED;
    }
    
    void VulkanDevice::WaitIdle() const
    {
        vkDeviceWaitIdle(m_Device);
    }
}
