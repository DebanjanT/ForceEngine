#pragma once

#include <Force/Core/Application.h>

namespace Force
{
    class EditorApp : public Application
    {
    public:
        EditorApp(const ApplicationConfig& config);
        ~EditorApp() override;
        
    protected:
        void OnInit() override;
        void OnShutdown() override;
        void OnUpdate(f32 deltaTime) override;
        void OnRender() override;
        void OnImGuiRender() override;
    };
}
