#pragma once

#include "Force/Core/Base.h"
#include "Force/Renderer/RendererTypes.h"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace Force
{
    class VulkanBuffer
    {
    public:
        VulkanBuffer();
        ~VulkanBuffer();
        
        void Create(usize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
        void Destroy();
        
        void SetData(const void* data, usize size, usize offset = 0);
        void* Map();
        void Unmap();
        
        VkBuffer GetBuffer() const { return m_Buffer; }
        VmaAllocation GetAllocation() const { return m_Allocation; }
        usize GetSize() const { return m_Size; }
        
        static void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
        
    private:
        VkBuffer m_Buffer = VK_NULL_HANDLE;
        VmaAllocation m_Allocation = VK_NULL_HANDLE;
        usize m_Size = 0;
        void* m_MappedData = nullptr;
    };
    
    class VertexBuffer
    {
    public:
        VertexBuffer();
        ~VertexBuffer();
        
        void Create(const void* vertices, usize size);
        void Destroy();
        
        void Bind(VkCommandBuffer commandBuffer) const;
        
        VkBuffer GetBuffer() const { return m_Buffer.GetBuffer(); }
        
    private:
        VulkanBuffer m_Buffer;
    };
    
    class IndexBuffer
    {
    public:
        IndexBuffer();
        ~IndexBuffer();
        
        void Create(const u32* indices, u32 count);
        void Destroy();
        
        void Bind(VkCommandBuffer commandBuffer) const;
        
        u32 GetCount() const { return m_Count; }
        VkBuffer GetBuffer() const { return m_Buffer.GetBuffer(); }
        
    private:
        VulkanBuffer m_Buffer;
        u32 m_Count = 0;
    };
    
    class UniformBuffer
    {
    public:
        UniformBuffer();
        ~UniformBuffer();
        
        void Create(usize size);
        void Destroy();
        
        void SetData(const void* data, usize size, usize offset = 0);
        
        VkBuffer GetBuffer() const { return m_Buffer.GetBuffer(); }
        VkDescriptorBufferInfo GetDescriptorInfo() const;
        
    private:
        VulkanBuffer m_Buffer;
    };
}
