#include "Force/Core/Memory/Allocators.h"
#include "Force/Core/Assert.h"
#include <cstdlib>
#include <cstring>

namespace Force
{
    // LinearAllocator
    LinearAllocator::LinearAllocator(usize size)
        : m_Size(size)
    {
        m_Memory = std::malloc(size);
        FORCE_CORE_ASSERT(m_Memory, "Failed to allocate linear allocator memory");
    }
    
    LinearAllocator::~LinearAllocator()
    {
        if (m_Memory)
        {
            std::free(m_Memory);
            m_Memory = nullptr;
        }
    }
    
    void* LinearAllocator::Allocate(usize size, usize alignment)
    {
        usize currentAddress = reinterpret_cast<usize>(m_Memory) + m_Offset;
        usize alignedAddress = (currentAddress + alignment - 1) & ~(alignment - 1);
        usize padding = alignedAddress - currentAddress;
        
        if (m_Offset + padding + size > m_Size)
        {
            FORCE_CORE_ERROR("LinearAllocator out of memory!");
            return nullptr;
        }
        
        m_Offset += padding + size;
        return reinterpret_cast<void*>(alignedAddress);
    }
    
    void LinearAllocator::Free(void* ptr)
    {
        // No-op for linear allocator
    }
    
    void LinearAllocator::Reset()
    {
        m_Offset = 0;
    }
    
    // PoolAllocator
    PoolAllocator::PoolAllocator(usize blockSize, usize blockCount)
        : m_BlockSize(blockSize < sizeof(FreeBlock) ? sizeof(FreeBlock) : blockSize)
        , m_BlockCount(blockCount)
    {
        m_Memory = std::malloc(m_BlockSize * blockCount);
        FORCE_CORE_ASSERT(m_Memory, "Failed to allocate pool allocator memory");
        
        Reset();
    }
    
    PoolAllocator::~PoolAllocator()
    {
        if (m_Memory)
        {
            std::free(m_Memory);
            m_Memory = nullptr;
        }
    }
    
    void* PoolAllocator::Allocate(usize size, usize alignment)
    {
        if (size > m_BlockSize)
        {
            FORCE_CORE_ERROR("PoolAllocator: requested size {} exceeds block size {}", size, m_BlockSize);
            return nullptr;
        }
        
        if (!m_FreeList)
        {
            FORCE_CORE_ERROR("PoolAllocator out of blocks!");
            return nullptr;
        }
        
        FreeBlock* block = m_FreeList;
        m_FreeList = m_FreeList->Next;
        m_AllocatedBlocks++;
        
        return block;
    }
    
    void PoolAllocator::Free(void* ptr)
    {
        if (!ptr) return;
        
        FreeBlock* block = static_cast<FreeBlock*>(ptr);
        block->Next = m_FreeList;
        m_FreeList = block;
        m_AllocatedBlocks--;
    }
    
    void PoolAllocator::Reset()
    {
        m_FreeList = static_cast<FreeBlock*>(m_Memory);
        FreeBlock* current = m_FreeList;
        
        for (usize i = 0; i < m_BlockCount - 1; ++i)
        {
            current->Next = reinterpret_cast<FreeBlock*>(
                reinterpret_cast<u8*>(current) + m_BlockSize);
            current = current->Next;
        }
        current->Next = nullptr;
        
        m_AllocatedBlocks = 0;
    }
    
    // FrameAllocator
    FrameAllocator::FrameAllocator(usize size)
        : m_Size(size)
    {
        m_Memory = std::malloc(size);
        FORCE_CORE_ASSERT(m_Memory, "Failed to allocate frame allocator memory");
    }
    
    FrameAllocator::~FrameAllocator()
    {
        if (m_Memory)
        {
            std::free(m_Memory);
            m_Memory = nullptr;
        }
    }
    
    void* FrameAllocator::Allocate(usize size, usize alignment)
    {
        usize currentAddress = reinterpret_cast<usize>(m_Memory) + m_Offset;
        usize alignedAddress = (currentAddress + alignment - 1) & ~(alignment - 1);
        usize padding = alignedAddress - currentAddress;
        
        if (m_Offset + padding + size > m_Size)
        {
            FORCE_CORE_ERROR("FrameAllocator out of memory!");
            return nullptr;
        }
        
        m_Offset += padding + size;
        return reinterpret_cast<void*>(alignedAddress);
    }
    
    void FrameAllocator::Free(void* ptr)
    {
        // No-op for frame allocator
    }
    
    void FrameAllocator::Reset()
    {
        m_Offset = 0;
    }
    
    void FrameAllocator::BeginFrame()
    {
        Reset();
    }
    
    void FrameAllocator::EndFrame()
    {
        // Could add debug checks here
    }
    
    // Global allocators
    static LinearAllocator* s_LinearAllocator = nullptr;
    static FrameAllocator* s_FrameAllocator = nullptr;
    
    LinearAllocator& GetLinearAllocator()
    {
        if (!s_LinearAllocator)
        {
            s_LinearAllocator = new LinearAllocator(64 * 1024 * 1024); // 64MB
        }
        return *s_LinearAllocator;
    }
    
    FrameAllocator& GetFrameAllocator()
    {
        if (!s_FrameAllocator)
        {
            s_FrameAllocator = new FrameAllocator(16 * 1024 * 1024); // 16MB
        }
        return *s_FrameAllocator;
    }
}
