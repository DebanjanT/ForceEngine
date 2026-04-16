#pragma once

#include "Force/Core/Types.h"
#include <cstdlib>

namespace Force
{
    // Base allocator interface
    class Allocator
    {
    public:
        virtual ~Allocator() = default;
        
        virtual void* Allocate(usize size, usize alignment = 8) = 0;
        virtual void Free(void* ptr) = 0;
        virtual void Reset() = 0;
        
        virtual usize GetUsedMemory() const = 0;
        virtual usize GetTotalMemory() const = 0;
    };
    
    // Linear/Stack allocator - fast, no individual frees
    class LinearAllocator : public Allocator
    {
    public:
        LinearAllocator(usize size);
        ~LinearAllocator() override;
        
        void* Allocate(usize size, usize alignment = 8) override;
        void Free(void* ptr) override; // No-op for linear allocator
        void Reset() override;
        
        usize GetUsedMemory() const override { return m_Offset; }
        usize GetTotalMemory() const override { return m_Size; }
        
    private:
        void* m_Memory = nullptr;
        usize m_Size = 0;
        usize m_Offset = 0;
    };
    
    // Pool allocator - fixed-size blocks
    class PoolAllocator : public Allocator
    {
    public:
        PoolAllocator(usize blockSize, usize blockCount);
        ~PoolAllocator() override;
        
        void* Allocate(usize size, usize alignment = 8) override;
        void Free(void* ptr) override;
        void Reset() override;
        
        usize GetUsedMemory() const override { return m_AllocatedBlocks * m_BlockSize; }
        usize GetTotalMemory() const override { return m_BlockCount * m_BlockSize; }
        
    private:
        struct FreeBlock
        {
            FreeBlock* Next;
        };
        
        void* m_Memory = nullptr;
        FreeBlock* m_FreeList = nullptr;
        usize m_BlockSize = 0;
        usize m_BlockCount = 0;
        usize m_AllocatedBlocks = 0;
    };
    
    // Frame allocator - resets every frame
    class FrameAllocator : public Allocator
    {
    public:
        FrameAllocator(usize size);
        ~FrameAllocator() override;
        
        void* Allocate(usize size, usize alignment = 8) override;
        void Free(void* ptr) override; // No-op
        void Reset() override;
        
        void BeginFrame();
        void EndFrame();
        
        usize GetUsedMemory() const override { return m_Offset; }
        usize GetTotalMemory() const override { return m_Size; }
        
    private:
        void* m_Memory = nullptr;
        usize m_Size = 0;
        usize m_Offset = 0;
    };
    
    // Global allocator access
    LinearAllocator& GetLinearAllocator();
    FrameAllocator& GetFrameAllocator();
}
