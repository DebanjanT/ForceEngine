#pragma once

#include "Force/Core/Base.h"
#include "Force/Platform/Window.h"

namespace Force
{
    struct ApplicationConfig
    {
        std::string Name = "Force Application";
        u32 WindowWidth = 1920;
        u32 WindowHeight = 1080;
        bool VSync = true;
        bool Fullscreen = false;
        bool EnableValidation = true;
    };

    class Application
    {
    public:
        Application(const ApplicationConfig& config);
        virtual ~Application();
        
        void Run();
        void Close();
        
        Window& GetWindow() { return *m_Window; }
        const Window& GetWindow() const { return *m_Window; }
        
        static Application& Get() { return *s_Instance; }
        
        const ApplicationConfig& GetConfig() const { return m_Config; }
        
        bool IsRunning() const { return m_Running; }
        
    protected:
        virtual void OnInit() {}
        virtual void OnShutdown() {}
        virtual void OnUpdate(f32 deltaTime) {}
        virtual void OnRender() {}
        virtual void OnImGuiRender() {}
        
    private:
        void Init();
        void Shutdown();
        
    private:
        ApplicationConfig m_Config;
        Scope<Window> m_Window;
        bool m_Running = true;
        bool m_Minimized = false;
        f32 m_LastFrameTime = 0.0f;
        
        static Application* s_Instance;
    };
    
    // To be defined by client
    Application* CreateApplication(int argc, char** argv);
}
