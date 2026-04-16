#pragma once

#include "Force/Core/Types.h"
#include <string>
#include <vector>

namespace Force
{
    class Platform
    {
    public:
        static void Init();
        static void Shutdown();
        
        // Time
        static f64 GetTime();
        static f64 GetAbsoluteTime();
        
        // Console
        static void ConsoleWrite(const char* message, u8 color);
        static void ConsoleWriteError(const char* message, u8 color);
        
        // Memory
        static void* Allocate(usize size, bool aligned);
        static void Free(void* block, bool aligned);
        static void* MemZero(void* block, usize size);
        static void* MemCopy(void* dest, const void* source, usize size);
        static void* MemSet(void* dest, i32 value, usize size);
        
        // File system
        static bool FileExists(const std::string& path);
        static std::vector<u8> ReadFileBytes(const std::string& path);
        static std::string ReadFileText(const std::string& path);
        static bool WriteFile(const std::string& path, const void* data, usize size);
        
        // Dynamic libraries
        static void* LoadDynamicLibrary(const std::string& path);
        static void UnloadDynamicLibrary(void* library);
        static void* GetDynamicLibraryFunction(void* library, const std::string& name);
        
        // Vulkan extensions
        static std::vector<const char*> GetRequiredVulkanExtensions();
    };
}
