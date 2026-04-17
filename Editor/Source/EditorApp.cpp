#include "EditorApp.h"
#include "EditorLayer.h"
#include <Force/Core/Logger.h>

namespace Force
{
    static ForceEditor::EditorLayer* s_Layer = nullptr;

    EditorApp::EditorApp(const ApplicationConfig& config)
        : Application(config)
    {
    }

    EditorApp::~EditorApp()
    {
        delete s_Layer;
    }

    void EditorApp::OnInit()
    {
        s_Layer = new ForceEditor::EditorLayer();
        s_Layer->OnAttach();
        FORCE_INFO("Force Editor initialized");
    }

    void EditorApp::OnShutdown()
    {
        s_Layer->OnDetach();
        FORCE_INFO("Force Editor shutting down");
    }

    void EditorApp::OnUpdate(f32 deltaTime)
    {
        s_Layer->OnUpdate(deltaTime);
    }

    void EditorApp::OnRender()
    {
        s_Layer->OnRender();
    }

    void EditorApp::OnImGuiRender()
    {
        s_Layer->OnImGuiRender();
    }
}

Force::Application* Force::CreateApplication(int argc, char** argv)
{
    Force::ApplicationConfig config;
    config.Name          = "Force Editor";
    config.WindowWidth   = 1920;
    config.WindowHeight  = 1080;
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
