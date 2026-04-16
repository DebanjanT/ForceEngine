#pragma once

#include <Force/Core/Base.h>

namespace Force
{
    class EditorLayer
    {
    public:
        EditorLayer();
        ~EditorLayer();
        
        void OnAttach();
        void OnDetach();
        void OnUpdate(f32 deltaTime);
        void OnRender();
        void OnImGuiRender();
        
    private:
        // Editor state
    };
}
