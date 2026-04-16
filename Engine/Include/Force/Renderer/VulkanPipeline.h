#pragma once

#include "Force/Core/Base.h"
#include "Force/Renderer/RendererTypes.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <string>

namespace Force
{
    struct PipelineConfig
    {
        std::string VertexShaderPath;
        std::string FragmentShaderPath;
        VkRenderPass RenderPass = VK_NULL_HANDLE;
        VkPipelineLayout PipelineLayout = VK_NULL_HANDLE;
        PipelineState State;
        
        std::vector<VkVertexInputBindingDescription> BindingDescriptions;
        std::vector<VkVertexInputAttributeDescription> AttributeDescriptions;
    };
    
    class VulkanPipeline
    {
    public:
        VulkanPipeline();
        ~VulkanPipeline();
        
        void Create(const PipelineConfig& config);
        void Destroy();
        
        void Bind(VkCommandBuffer commandBuffer);
        
        VkPipeline GetPipeline() const { return m_Pipeline; }
        VkPipelineLayout GetLayout() const { return m_Layout; }
        
        static std::vector<u8> ReadShaderFile(const std::string& filepath);
        static VkShaderModule CreateShaderModule(const std::vector<u8>& code);
        
        static std::vector<VkVertexInputBindingDescription> GetDefaultBindingDescriptions();
        static std::vector<VkVertexInputAttributeDescription> GetDefaultAttributeDescriptions();
        
    private:
        VkPipeline m_Pipeline = VK_NULL_HANDLE;
        VkPipelineLayout m_Layout = VK_NULL_HANDLE;
        bool m_OwnsLayout = false;
    };
}
