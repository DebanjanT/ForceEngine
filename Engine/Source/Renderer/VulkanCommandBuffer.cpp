#include "Force/Renderer/VulkanCommandBuffer.h"
#include "Force/Renderer/VulkanDevice.h"
#include "Force/Core/Logger.h"

namespace Force
{
    VulkanCommandBuffer::VulkanCommandBuffer() = default;
    VulkanCommandBuffer::~VulkanCommandBuffer() { Free(); }
    
    void VulkanCommandBuffer::Allocate(VkCommandPool pool, bool primary)
    {
        m_Pool = pool;
        
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = pool;
        allocInfo.level = primary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
        allocInfo.commandBufferCount = 1;
        
        VkResult result = vkAllocateCommandBuffers(VulkanDevice::Get().GetDevice(), &allocInfo, &m_CommandBuffer);
        FORCE_CORE_ASSERT(result == VK_SUCCESS, "Failed to allocate command buffer!");
    }
    
    void VulkanCommandBuffer::Free()
    {
        if (m_CommandBuffer != VK_NULL_HANDLE && m_Pool != VK_NULL_HANDLE)
        {
            vkFreeCommandBuffers(VulkanDevice::Get().GetDevice(), m_Pool, 1, &m_CommandBuffer);
            m_CommandBuffer = VK_NULL_HANDLE;
        }
    }
    
    void VulkanCommandBuffer::Begin(VkCommandBufferUsageFlags flags)
    {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = flags;
        
        VkResult result = vkBeginCommandBuffer(m_CommandBuffer, &beginInfo);
        FORCE_CORE_ASSERT(result == VK_SUCCESS, "Failed to begin recording command buffer!");
    }
    
    void VulkanCommandBuffer::End()
    {
        VkResult result = vkEndCommandBuffer(m_CommandBuffer);
        FORCE_CORE_ASSERT(result == VK_SUCCESS, "Failed to record command buffer!");
    }
    
    void VulkanCommandBuffer::Reset()
    {
        vkResetCommandBuffer(m_CommandBuffer, 0);
    }
    
    void VulkanCommandBuffer::BeginRenderPass(VkRenderPass renderPass, VkFramebuffer framebuffer,
                                              VkExtent2D extent, const VkClearValue* clearValues, u32 clearValueCount)
    {
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = framebuffer;
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = extent;
        renderPassInfo.clearValueCount = clearValueCount;
        renderPassInfo.pClearValues = clearValues;
        
        vkCmdBeginRenderPass(m_CommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    }
    
    void VulkanCommandBuffer::EndRenderPass()
    {
        vkCmdEndRenderPass(m_CommandBuffer);
    }
    
    void VulkanCommandBuffer::SetViewport(f32 x, f32 y, f32 width, f32 height, f32 minDepth, f32 maxDepth)
    {
        VkViewport viewport{};
        viewport.x = x;
        viewport.y = y;
        viewport.width = width;
        viewport.height = height;
        viewport.minDepth = minDepth;
        viewport.maxDepth = maxDepth;
        
        vkCmdSetViewport(m_CommandBuffer, 0, 1, &viewport);
    }
    
    void VulkanCommandBuffer::SetScissor(i32 x, i32 y, u32 width, u32 height)
    {
        VkRect2D scissor{};
        scissor.offset = { x, y };
        scissor.extent = { width, height };
        
        vkCmdSetScissor(m_CommandBuffer, 0, 1, &scissor);
    }
    
    void VulkanCommandBuffer::BindPipeline(VkPipeline pipeline, VkPipelineBindPoint bindPoint)
    {
        vkCmdBindPipeline(m_CommandBuffer, bindPoint, pipeline);
    }
    
    void VulkanCommandBuffer::BindVertexBuffer(VkBuffer buffer, VkDeviceSize offset)
    {
        vkCmdBindVertexBuffers(m_CommandBuffer, 0, 1, &buffer, &offset);
    }
    
    void VulkanCommandBuffer::BindIndexBuffer(VkBuffer buffer, VkIndexType indexType)
    {
        vkCmdBindIndexBuffer(m_CommandBuffer, buffer, 0, indexType);
    }
    
    void VulkanCommandBuffer::BindDescriptorSets(VkPipelineLayout layout, u32 firstSet,
                                                  u32 setCount, const VkDescriptorSet* sets,
                                                  u32 dynamicOffsetCount, const u32* dynamicOffsets)
    {
        vkCmdBindDescriptorSets(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout,
                                firstSet, setCount, sets, dynamicOffsetCount, dynamicOffsets);
    }
    
    void VulkanCommandBuffer::Draw(u32 vertexCount, u32 instanceCount, u32 firstVertex, u32 firstInstance)
    {
        vkCmdDraw(m_CommandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
    }
    
    void VulkanCommandBuffer::DrawIndexed(u32 indexCount, u32 instanceCount, u32 firstIndex,
                                          i32 vertexOffset, u32 firstInstance)
    {
        vkCmdDrawIndexed(m_CommandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }
    
    void VulkanCommandBuffer::PushConstants(VkPipelineLayout layout, VkShaderStageFlags stageFlags,
                                            u32 offset, u32 size, const void* data)
    {
        vkCmdPushConstants(m_CommandBuffer, layout, stageFlags, offset, size, data);
    }
    
    VkCommandBuffer VulkanCommandBuffer::BeginSingleTimeCommands()
    {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = VulkanDevice::Get().GetCommandPool();
        allocInfo.commandBufferCount = 1;
        
        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(VulkanDevice::Get().GetDevice(), &allocInfo, &commandBuffer);
        
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        
        vkBeginCommandBuffer(commandBuffer, &beginInfo);
        
        return commandBuffer;
    }
    
    void VulkanCommandBuffer::EndSingleTimeCommands(VkCommandBuffer commandBuffer)
    {
        vkEndCommandBuffer(commandBuffer);
        
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        
        vkQueueSubmit(VulkanDevice::Get().GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(VulkanDevice::Get().GetGraphicsQueue());
        
        vkFreeCommandBuffers(VulkanDevice::Get().GetDevice(), VulkanDevice::Get().GetCommandPool(), 1, &commandBuffer);
    }
}
