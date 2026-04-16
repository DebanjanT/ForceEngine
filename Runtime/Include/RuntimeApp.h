#pragma once

#include <Force/Core/Application.h>

namespace Force
{
    class RuntimeApp : public Application
    {
    public:
        RuntimeApp(const ApplicationConfig& config);
        ~RuntimeApp() override;
        
    protected:
        void OnInit() override;
        void OnShutdown() override;
        void OnUpdate(f32 deltaTime) override;
        void OnRender() override;
    };
}
