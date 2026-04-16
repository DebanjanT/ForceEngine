#pragma once

#include "Force/Core/Types.h"
#include <cstdlib>
#include <new>

namespace Force
{
    // Memory tracking statistics
    struct MemoryStats
    {
        usize TotalAllocated = 0;
        usize TotalFreed = 0;
        usize CurrentUsage = 0;
        usize PeakUsage = 0;
        u64 AllocationCount = 0;
        u64 FreeCount = 0;
    };
    
    class Memory
    {
    public:
        static void* Allocate(usize size, usize alignment = alignof(std::max_align_t));
        static void Free(void* ptr, usize size);
        
        static void* AllocateTagged(usize size, const char* tag);
        static void FreeTagged(void* ptr, usize size, const char* tag);
        
        static const MemoryStats& GetStats();
        static void ResetStats();
        
    private:
        static MemoryStats s_Stats;
    };
    
    // Aligned allocation helpers
    template<typename T, typename... Args>
    T* New(Args&&... args)
    {
        void* ptr = Memory::Allocate(sizeof(T), alignof(T));
        return new(ptr) T(std::forward<Args>(args)...);
    }
    
    template<typename T>
    void Delete(T* ptr)
    {
        if (ptr)
        {
            ptr->~T();
            Memory::Free(ptr, sizeof(T));
        }
    }
}
