#include "EditorApp.h"
#include "EditorLayer.h"
#include <Force/Core/Logger.h>

namespace Force
{
    EditorApp::EditorApp(const ApplicationConfig& config)
        : Application(config)
    {
    }
    
    EditorApp::~EditorApp()
    {
    }
    
    void EditorApp::OnInit()
    {
        FORCE_INFO("Force Editor initialized");
    }
    
    void EditorApp::OnShutdown()
    {
        FORCE_INFO("Force Editor shutting down");
    }
    
    void EditorApp::OnUpdate(f32 deltaTime)
    {
        // Update editor systems
    }
    
    void EditorApp::OnRender()
    {
        // Render scene
    }
    
    void EditorApp::OnImGuiRender()
    {
        // Render editor UI
    }
}

// Entry point
Force::Application* Force::CreateApplication(int argc, char** argv)
{
    Force::ApplicationConfig config;
    config.Name = "Force Editor";
    config.WindowWidth = 1920;
    config.WindowHeight = 1080;
    config.EnableValidation = true;
    
    return new Force::EditorApp(config);
}

int main(int argc, char** argv)
{
    auto app = Force::CreateApplication(argc, argv);
    app->Run();
    delete app;
    return 0;
}
