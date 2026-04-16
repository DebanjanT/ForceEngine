#include "Force/Core/Engine.h"
#include "Force/Core/Logger.h"
#include "Force/Platform/Platform.h"

namespace Force
{
    f64 Engine::s_StartTime = 0.0;
    f64 Engine::s_LastFrameTime = 0.0;
    f64 Engine::s_DeltaTime = 0.0;
    
    void Engine::Init()
    {
        Logger::Init();
        Platform::Init();
        
        s_StartTime = Platform::GetAbsoluteTime();
        s_LastFrameTime = s_StartTime;
        
        FORCE_CORE_INFO("Force Engine initialized");
    }
    
    void Engine::Shutdown()
    {
        FORCE_CORE_INFO("Force Engine shutting down");
        
        Platform::Shutdown();
        Logger::Shutdown();
    }
    
    f64 Engine::GetTime()
    {
        return Platform::GetAbsoluteTime() - s_StartTime;
    }
    
    f64 Engine::GetDeltaTime()
    {
        return s_DeltaTime;
    }
}
