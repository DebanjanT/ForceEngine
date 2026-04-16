#include "Force/Platform/Platform.h"
#include "Force/Core/Logger.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#ifdef FORCE_PLATFORM_WINDOWS
    #include <Windows.h>
#endif

#include <fstream>
#include <chrono>
#include <cstring>

namespace Force
{
    static f64 s_Frequency = 0.0;
    static f64 s_StartTime = 0.0;
    
    void Platform::Init()
    {
        #ifdef FORCE_PLATFORM_WINDOWS
        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);
        s_Frequency = 1.0 / static_cast<f64>(frequency.QuadPart);
        
        LARGE_INTEGER counter;
        QueryPerformanceCounter(&counter);
        s_StartTime = static_cast<f64>(counter.QuadPart) * s_Frequency;
        #else
        auto now = std::chrono::high_resolution_clock::now();
        s_StartTime = std::chrono::duration<f64>(now.time_since_epoch()).count();
        #endif
        
        FORCE_CORE_INFO("Platform layer initialized");
    }
    
    void Platform::Shutdown()
    {
        FORCE_CORE_INFO("Platform layer shutdown");
    }
    
    f64 Platform::GetTime()
    {
        return GetAbsoluteTime() - s_StartTime;
    }
    
    f64 Platform::GetAbsoluteTime()
    {
        #ifdef FORCE_PLATFORM_WINDOWS
        LARGE_INTEGER counter;
        QueryPerformanceCounter(&counter);
        return static_cast<f64>(counter.QuadPart) * s_Frequency;
        #else
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<f64>(now.time_since_epoch()).count();
        #endif
    }
    
    void Platform::ConsoleWrite(const char* message, u8 color)
    {
        #ifdef FORCE_PLATFORM_WINDOWS
        HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
        static u8 levels[6] = { 64, 4, 6, 2, 1, 8 };
        SetConsoleTextAttribute(consoleHandle, levels[color]);
        OutputDebugStringA(message);
        DWORD written = 0;
        WriteConsoleA(consoleHandle, message, static_cast<DWORD>(strlen(message)), &written, nullptr);
        SetConsoleTextAttribute(consoleHandle, 7);
        #else
        const char* colorStrings[] = { "0;41", "1;31", "1;33", "1;32", "1;34", "1;30" };
        printf("\033[%sm%s\033[0m", colorStrings[color], message);
        #endif
    }
    
    void Platform::ConsoleWriteError(const char* message, u8 color)
    {
        #ifdef FORCE_PLATFORM_WINDOWS
        HANDLE consoleHandle = GetStdHandle(STD_ERROR_HANDLE);
        static u8 levels[6] = { 64, 4, 6, 2, 1, 8 };
        SetConsoleTextAttribute(consoleHandle, levels[color]);
        OutputDebugStringA(message);
        DWORD written = 0;
        WriteConsoleA(consoleHandle, message, static_cast<DWORD>(strlen(message)), &written, nullptr);
        SetConsoleTextAttribute(consoleHandle, 7);
        #else
        const char* colorStrings[] = { "0;41", "1;31", "1;33", "1;32", "1;34", "1;30" };
        fprintf(stderr, "\033[%sm%s\033[0m", colorStrings[color], message);
        #endif
    }
    
    void* Platform::Allocate(usize size, bool aligned)
    {
        #ifdef FORCE_PLATFORM_WINDOWS
        return aligned ? _aligned_malloc(size, 16) : malloc(size);
        #else
        return aligned ? aligned_alloc(16, size) : malloc(size);
        #endif
    }
    
    void Platform::Free(void* block, bool aligned)
    {
        #ifdef FORCE_PLATFORM_WINDOWS
        if (aligned)
            _aligned_free(block);
        else
            free(block);
        #else
        free(block);
        #endif
    }
    
    void* Platform::MemZero(void* block, usize size)
    {
        return ::memset(block, 0, size);
    }
    
    void* Platform::MemCopy(void* dest, const void* source, usize size)
    {
        return ::memcpy(dest, source, size);
    }
    
    void* Platform::MemSet(void* dest, i32 value, usize size)
    {
        return ::memset(dest, value, size);
    }
    
    bool Platform::FileExists(const std::string& path)
    {
        std::ifstream file(path);
        return file.good();
    }
    
    std::vector<u8> Platform::ReadFileBytes(const std::string& path)
    {
        std::ifstream file(path, std::ios::ate | std::ios::binary);
        
        if (!file.is_open())
        {
            FORCE_CORE_ERROR("Failed to open file: {}", path);
            return {};
        }
        
        usize fileSize = static_cast<usize>(file.tellg());
        std::vector<u8> buffer(fileSize);
        
        file.seekg(0);
        file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
        file.close();
        
        return buffer;
    }
    
    std::string Platform::ReadFileText(const std::string& path)
    {
        std::ifstream file(path);
        
        if (!file.is_open())
        {
            FORCE_CORE_ERROR("Failed to open file: {}", path);
            return "";
        }
        
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        file.close();
        
        return content;
    }
    
    bool Platform::WriteFile(const std::string& path, const void* data, usize size)
    {
        std::ofstream file(path, std::ios::binary);
        
        if (!file.is_open())
        {
            FORCE_CORE_ERROR("Failed to create file: {}", path);
            return false;
        }
        
        file.write(static_cast<const char*>(data), size);
        file.close();
        
        return true;
    }
    
    void* Platform::LoadDynamicLibrary(const std::string& path)
    {
        #ifdef FORCE_PLATFORM_WINDOWS
        return LoadLibraryA(path.c_str());
        #else
        return dlopen(path.c_str(), RTLD_LAZY);
        #endif
    }
    
    void Platform::UnloadDynamicLibrary(void* library)
    {
        #ifdef FORCE_PLATFORM_WINDOWS
        FreeLibrary(static_cast<HMODULE>(library));
        #else
        dlclose(library);
        #endif
    }
    
    void* Platform::GetDynamicLibraryFunction(void* library, const std::string& name)
    {
        #ifdef FORCE_PLATFORM_WINDOWS
        return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(library), name.c_str()));
        #else
        return dlsym(library, name.c_str());
        #endif
    }
    
    std::vector<const char*> Platform::GetRequiredVulkanExtensions()
    {
        u32 glfwExtensionCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        
        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
        
        #ifdef FORCE_VULKAN_VALIDATION
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        #endif
        
        return extensions;
    }
}
