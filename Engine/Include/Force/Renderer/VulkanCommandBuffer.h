#pragma once

#include "Force/Core/Base.h"
#include <vulkan/vulkan.h>

namespace Force
{
    class VulkanCommandBuffer
    {
    public:
        VulkanCommandBuffer();
        ~VulkanCommandBuffer();
        
        void Allocate(VkCommandPool pool, bool primary = true);
        void Free();
        
        void Begin(VkCommandBufferUsageFlags flags = 0);
        void End();
        void Reset();
        
        void BeginRenderPass(VkRenderPass renderPass, VkFramebuffer framebuffer, 
                            VkExtent2D extent, const VkClearValue* clearValues, u32 clearValueCount);
        void EndRenderPass();
        
        void SetViewport(f32 x, f32 y, f32 width, f32 height, f32 minDepth = 0.0f, f32 maxDepth = 1.0f);
        void SetScissor(i32 x, i32 y, u32 width, u32 height);
        
        void BindPipeline(VkPipeline pipeline, VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS);
        void BindVertexBuffer(VkBuffer buffer, VkDeviceSize offset = 0);
        void BindIndexBuffer(VkBuffer buffer, VkIndexType indexType = VK_INDEX_TYPE_UINT32);
        void BindDescriptorSets(VkPipelineLayout layout, u32 firstSet, 
                               u32 setCount, const VkDescriptorSet* sets,
                               u32 dynamicOffsetCount = 0, const u32* dynamicOffsets = nullptr);
        
        void Draw(u32 vertexCount, u32 instanceCount = 1, u32 firstVertex = 0, u32 firstInstance = 0);
        void DrawIndexed(u32 indexCount, u32 instanceCount = 1, u32 firstIndex = 0, 
                        i32 vertexOffset = 0, u32 firstInstance = 0);
        
        void PushConstants(VkPipelineLayout layout, VkShaderStageFlags stageFlags, 
                          u32 offset, u32 size, const void* data);
        
        VkCommandBuffer GetHandle() const { return m_CommandBuffer; }
        
        static VkCommandBuffer BeginSingleTimeCommands();
        static void EndSingleTimeCommands(VkCommandBuffer commandBuffer);
        
    private:
        VkCommandBuffer m_CommandBuffer = VK_NULL_HANDLE;
        VkCommandPool m_Pool = VK_NULL_HANDLE;
    };
}
