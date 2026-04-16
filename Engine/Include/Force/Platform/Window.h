#pragma once

#include "Force/Core/Types.h"
#include <string>
#include <functional>

struct GLFWwindow;

namespace Force
{
    struct WindowProps
    {
        std::string Title = "Force Engine";
        u32 Width = 1920;
        u32 Height = 1080;
        bool VSync = true;
        bool Fullscreen = false;
        bool Resizable = true;
    };
    
    class Window
    {
    public:
        using EventCallbackFn = std::function<void(class Event&)>;
        
        Window(const WindowProps& props);
        ~Window();
        
        void OnUpdate();
        
        u32 GetWidth() const { return m_Data.Width; }
        u32 GetHeight() const { return m_Data.Height; }
        
        void SetVSync(bool enabled);
        bool IsVSync() const { return m_Data.VSync; }
        
        void SetEventCallback(const EventCallbackFn& callback) { m_Data.EventCallback = callback; }
        
        GLFWwindow* GetNativeWindow() const { return m_Window; }
        
        bool ShouldClose() const;
        
        void SetTitle(const std::string& title);
        
        // Vulkan surface creation
        void CreateVulkanSurface(void* instance, void* surface);
        
        static Scope<Window> Create(const WindowProps& props = WindowProps());
        
    private:
        void Init(const WindowProps& props);
        void Shutdown();
        
    private:
        GLFWwindow* m_Window = nullptr;
        
        struct WindowData
        {
            std::string Title;
            u32 Width = 0;
            u32 Height = 0;
            bool VSync = true;
            EventCallbackFn EventCallback;
        };
        
        WindowData m_Data;
    };
}
