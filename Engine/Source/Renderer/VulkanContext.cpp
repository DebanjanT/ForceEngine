#include "Force/Renderer/VulkanContext.h"
#include "Force/Platform/Platform.h"
#include "Force/Core/Logger.h"

namespace Force
{
    VulkanContext* VulkanContext::s_Instance = nullptr;
    
    VulkanContext::VulkanContext()
    {
        FORCE_CORE_ASSERT(!s_Instance, "VulkanContext already exists!");
        s_Instance = this;
    }
    
    VulkanContext::~VulkanContext()
    {
        s_Instance = nullptr;
    }
    
    void VulkanContext::Init(const RendererConfig& config)
    {
        m_ValidationEnabled = config.EnableValidation;
        
        FORCE_CORE_INFO("Initializing Vulkan context...");
        
        CreateInstance();
        
        if (m_ValidationEnabled)
        {
            SetupDebugMessenger();
        }
        
        FORCE_CORE_INFO("Vulkan context initialized successfully");
    }
    
    void VulkanContext::Shutdown()
    {
        FORCE_CORE_INFO("Shutting down Vulkan context...");
        
        if (m_ValidationEnabled && m_DebugMessenger != VK_NULL_HANDLE)
        {
            auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
                vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugUtilsMessengerEXT"));
            if (func)
            {
                func(m_Instance, m_DebugMessenger, nullptr);
            }
        }
        
        if (m_Surface != VK_NULL_HANDLE)
        {
            vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
        }
        
        if (m_Instance != VK_NULL_HANDLE)
        {
            vkDestroyInstance(m_Instance, nullptr);
        }
    }
    
    void VulkanContext::CreateInstance()
    {
        if (m_ValidationEnabled && !CheckValidationLayerSupport())
        {
            FORCE_CORE_WARN("Validation layers requested but not available!");
            m_ValidationEnabled = false;
        }
        
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Force Engine Application";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "Force Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
        appInfo.apiVersion = VK_API_VERSION_1_3;
        
        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        
        auto extensions = GetRequiredExtensions();
        createInfo.enabledExtensionCount = static_cast<u32>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();
        
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        if (m_ValidationEnabled)
        {
            createInfo.enabledLayerCount = static_cast<u32>(m_ValidationLayers.size());
            createInfo.ppEnabledLayerNames = m_ValidationLayers.data();
            
            debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                              VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                              VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                          VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                          VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            debugCreateInfo.pfnUserCallback = DebugCallback;
            
            createInfo.pNext = &debugCreateInfo;
        }
        else
        {
            createInfo.enabledLayerCount = 0;
            createInfo.pNext = nullptr;
        }
        
        VkResult result = vkCreateInstance(&createInfo, nullptr, &m_Instance);
        FORCE_CORE_ASSERT(result == VK_SUCCESS, "Failed to create Vulkan instance!");
        
        FORCE_CORE_INFO("Vulkan instance created");
    }
    
    void VulkanContext::SetupDebugMessenger()
    {
        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                     VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                     VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = DebugCallback;
        
        auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(m_Instance, "vkCreateDebugUtilsMessengerEXT"));
        
        if (func)
        {
            VkResult result = func(m_Instance, &createInfo, nullptr, &m_DebugMessenger);
            FORCE_CORE_ASSERT(result == VK_SUCCESS, "Failed to set up debug messenger!");
        }
    }
    
    bool VulkanContext::CheckValidationLayerSupport()
    {
        u32 layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        
        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
        
        for (const char* layerName : m_ValidationLayers)
        {
            bool layerFound = false;
            
            for (const auto& layerProperties : availableLayers)
            {
                if (strcmp(layerName, layerProperties.layerName) == 0)
                {
                    layerFound = true;
                    break;
                }
            }
            
            if (!layerFound)
            {
                return false;
            }
        }
        
        return true;
    }
    
    std::vector<const char*> VulkanContext::GetRequiredExtensions()
    {
        return Platform::GetRequiredVulkanExtensions();
    }
    
    VKAPI_ATTR VkBool32 VKAPI_CALL VulkanContext::DebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData)
    {
        switch (messageSeverity)
        {
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
                FORCE_CORE_TRACE("Vulkan: {}", pCallbackData->pMessage);
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
                FORCE_CORE_INFO("Vulkan: {}", pCallbackData->pMessage);
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
                FORCE_CORE_WARN("Vulkan: {}", pCallbackData->pMessage);
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
                FORCE_CORE_ERROR("Vulkan: {}", pCallbackData->pMessage);
                break;
            default:
                break;
        }
        
        return VK_FALSE;
    }
}
