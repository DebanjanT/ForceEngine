#include "Force/Renderer/VulkanBuffer.h"
#include "Force/Renderer/VulkanDevice.h"
#include "Force/Renderer/VulkanCommandBuffer.h"
#include "Force/Core/Logger.h"
#include <cstring>

namespace Force
{
    // VulkanBuffer
    VulkanBuffer::VulkanBuffer() = default;
    VulkanBuffer::~VulkanBuffer() { Destroy(); }
    
    void VulkanBuffer::Create(usize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage)
    {
        m_Size = size;
        
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        
        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = memoryUsage;
        
        VkResult result = vmaCreateBuffer(VulkanDevice::Get().GetAllocator(), &bufferInfo, &allocInfo,
                                          &m_Buffer, &m_Allocation, nullptr);
        FORCE_CORE_ASSERT(result == VK_SUCCESS, "Failed to create buffer!");
    }
    
    void VulkanBuffer::Destroy()
    {
        if (m_Buffer != VK_NULL_HANDLE)
        {
            vmaDestroyBuffer(VulkanDevice::Get().GetAllocator(), m_Buffer, m_Allocation);
            m_Buffer = VK_NULL_HANDLE;
            m_Allocation = VK_NULL_HANDLE;
        }
    }
    
    void VulkanBuffer::SetData(const void* data, usize size, usize offset)
    {
        void* mapped = Map();
        memcpy(static_cast<u8*>(mapped) + offset, data, size);
        Unmap();
    }
    
    void* VulkanBuffer::Map()
    {
        if (!m_MappedData)
        {
            vmaMapMemory(VulkanDevice::Get().GetAllocator(), m_Allocation, &m_MappedData);
        }
        return m_MappedData;
    }
    
    void VulkanBuffer::Unmap()
    {
        if (m_MappedData)
        {
            vmaUnmapMemory(VulkanDevice::Get().GetAllocator(), m_Allocation);
            m_MappedData = nullptr;
        }
    }
    
    void VulkanBuffer::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
    {
        VkCommandBuffer commandBuffer = VulkanCommandBuffer::BeginSingleTimeCommands();
        
        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
        
        VulkanCommandBuffer::EndSingleTimeCommands(commandBuffer);
    }
    
    // VertexBuffer
    VertexBuffer::VertexBuffer() = default;
    VertexBuffer::~VertexBuffer() { Destroy(); }
    
    void VertexBuffer::Create(const void* vertices, usize size)
    {
        VulkanBuffer stagingBuffer;
        stagingBuffer.Create(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
        stagingBuffer.SetData(vertices, size);
        
        m_Buffer.Create(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                       VMA_MEMORY_USAGE_GPU_ONLY);
        
        VulkanBuffer::CopyBuffer(stagingBuffer.GetBuffer(), m_Buffer.GetBuffer(), size);
        stagingBuffer.Destroy();
    }
    
    void VertexBuffer::Destroy()
    {
        m_Buffer.Destroy();
    }
    
    void VertexBuffer::Bind(VkCommandBuffer commandBuffer) const
    {
        VkBuffer buffers[] = { m_Buffer.GetBuffer() };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
    }
    
    // IndexBuffer
    IndexBuffer::IndexBuffer() = default;
    IndexBuffer::~IndexBuffer() { Destroy(); }
    
    void IndexBuffer::Create(const u32* indices, u32 count)
    {
        m_Count = count;
        usize size = count * sizeof(u32);
        
        VulkanBuffer stagingBuffer;
        stagingBuffer.Create(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
        stagingBuffer.SetData(indices, size);
        
        m_Buffer.Create(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                       VMA_MEMORY_USAGE_GPU_ONLY);
        
        VulkanBuffer::CopyBuffer(stagingBuffer.GetBuffer(), m_Buffer.GetBuffer(), size);
        stagingBuffer.Destroy();
    }
    
    void IndexBuffer::Destroy()
    {
        m_Buffer.Destroy();
    }
    
    void IndexBuffer::Bind(VkCommandBuffer commandBuffer) const
    {
        vkCmdBindIndexBuffer(commandBuffer, m_Buffer.GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
    }
    
    // UniformBuffer
    UniformBuffer::UniformBuffer() = default;
    UniformBuffer::~UniformBuffer() { Destroy(); }
    
    void UniformBuffer::Create(usize size)
    {
        m_Buffer.Create(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    }
    
    void UniformBuffer::Destroy()
    {
        m_Buffer.Destroy();
    }
    
    void UniformBuffer::SetData(const void* data, usize size, usize offset)
    {
        m_Buffer.SetData(data, size, offset);
    }
    
    VkDescriptorBufferInfo UniformBuffer::GetDescriptorInfo() const
    {
        VkDescriptorBufferInfo info{};
        info.buffer = m_Buffer.GetBuffer();
        info.offset = 0;
        info.range = m_Buffer.GetSize();
        return info;
    }
}
