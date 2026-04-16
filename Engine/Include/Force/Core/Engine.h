#pragma once

#include "Force/Core/Base.h"

namespace Force
{
    class Engine
    {
    public:
        static void Init();
        static void Shutdown();
        
        static f64 GetTime();
        static f64 GetDeltaTime();
        
    private:
        static f64 s_StartTime;
        static f64 s_LastFrameTime;
        static f64 s_DeltaTime;
    };
}
