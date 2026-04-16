#pragma once

#include "Force/Core/Base.h"
#include "Force/Renderer/RendererTypes.h"
#include <vulkan/vulkan.h>
#include <vector>

namespace Force
{
    class VulkanContext
    {
    public:
        VulkanContext();
        ~VulkanContext();
        
        void Init(const RendererConfig& config);
        void Shutdown();
        
        VkInstance GetInstance() const { return m_Instance; }
        VkSurfaceKHR GetSurface() const { return m_Surface; }
        
        void SetSurface(VkSurfaceKHR surface) { m_Surface = surface; }
        
        bool IsValidationEnabled() const { return m_ValidationEnabled; }
        
        static VulkanContext& Get() { return *s_Instance; }
        
    private:
        void CreateInstance();
        void SetupDebugMessenger();
        bool CheckValidationLayerSupport();
        std::vector<const char*> GetRequiredExtensions();
        
        static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
            void* pUserData);
        
    private:
        VkInstance m_Instance = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
        VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
        
        bool m_ValidationEnabled = false;
        
        static VulkanContext* s_Instance;
        
        const std::vector<const char*> m_ValidationLayers = {
            "VK_LAYER_KHRONOS_validation"
        };
    };
}
