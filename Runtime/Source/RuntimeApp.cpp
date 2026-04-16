#include "RuntimeApp.h"
#include <Force/Core/Logger.h>

namespace Force
{
    RuntimeApp::RuntimeApp(const ApplicationConfig& config)
        : Application(config)
    {
    }
    
    RuntimeApp::~RuntimeApp()
    {
    }
    
    void RuntimeApp::OnInit()
    {
        FORCE_INFO("Force Runtime initialized");
    }
    
    void RuntimeApp::OnShutdown()
    {
        FORCE_INFO("Force Runtime shutting down");
    }
    
    void RuntimeApp::OnUpdate(f32 deltaTime)
    {
        // Update game systems
    }
    
    void RuntimeApp::OnRender()
    {
        // Render game
    }
}

// Entry point
Force::Application* Force::CreateApplication(int argc, char** argv)
{
    Force::ApplicationConfig config;
    config.Name = "Force Runtime";
    config.WindowWidth = 1920;
    config.WindowHeight = 1080;
    config.EnableValidation = false; // No validation in shipping builds
    
    return new Force::RuntimeApp(config);
}

int main(int argc, char** argv)
{
    auto app = Force::CreateApplication(argc, argv);
    app->Run();
    delete app;
    return 0;
}
