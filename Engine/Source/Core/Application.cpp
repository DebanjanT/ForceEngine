#include "Force/Core/Application.h"
#include "Force/Core/Engine.h"
#include "Force/Renderer/Renderer.h"
#include "Force/Platform/Platform.h"

namespace Force
{
    Application* Application::s_Instance = nullptr;
    
    Application::Application(const ApplicationConfig& config)
        : m_Config(config)
    {
        FORCE_CORE_ASSERT(!s_Instance, "Application already exists!");
        s_Instance = this;
    }
    
    Application::~Application()
    {
        s_Instance = nullptr;
    }
    
    void Application::Run()
    {
        Init();
        
        while (m_Running)
        {
            f32 time = static_cast<f32>(Platform::GetTime());
            f32 deltaTime = time - m_LastFrameTime;
            m_LastFrameTime = time;
            
            if (!m_Minimized)
            {
                OnUpdate(deltaTime);
                
                Renderer::BeginFrame();
                OnRender();
                OnImGuiRender();
                Renderer::EndFrame();
            }
            
            m_Window->OnUpdate();
            
            FORCE_PROFILE_FRAME_MARK;
        }
        
        Shutdown();
    }
    
    void Application::Close()
    {
        m_Running = false;
    }
    
    void Application::Init()
    {
        Engine::Init();
        
        FORCE_CORE_INFO("Creating application: {}", m_Config.Name);
        
        WindowProps windowProps;
        windowProps.Title = m_Config.Name;
        windowProps.Width = m_Config.WindowWidth;
        windowProps.Height = m_Config.WindowHeight;
        windowProps.VSync = m_Config.VSync;
        windowProps.Fullscreen = m_Config.Fullscreen;
        
        m_Window = Window::Create(windowProps);
        
        RendererConfig rendererConfig;
        rendererConfig.EnableValidation = m_Config.EnableValidation;
        rendererConfig.VSync = m_Config.VSync;
        
        Renderer::Init(rendererConfig);
        
        OnInit();
    }
    
    void Application::Shutdown()
    {
        OnShutdown();
        
        Renderer::Shutdown();
        m_Window.reset();
        
        Engine::Shutdown();
    }
}
