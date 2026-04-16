#pragma once

#include "Force/Core/Types.h"
#include "Force/Platform/KeyCodes.h"
#include <glm/glm.hpp>

namespace Force
{
    class Input
    {
    public:
        static void Init();
        static void Update();
        
        // Keyboard
        static bool IsKeyDown(KeyCode key);
        static bool IsKeyUp(KeyCode key);
        static bool WasKeyPressed(KeyCode key);
        static bool WasKeyReleased(KeyCode key);
        
        // Mouse
        static bool IsMouseButtonDown(MouseButton button);
        static bool IsMouseButtonUp(MouseButton button);
        static bool WasMouseButtonPressed(MouseButton button);
        static bool WasMouseButtonReleased(MouseButton button);
        
        static glm::vec2 GetMousePosition();
        static glm::vec2 GetMouseDelta();
        static f32 GetMouseScrollDelta();
        
        // Gamepad (future)
        static bool IsGamepadConnected(i32 index);
        static f32 GetGamepadAxis(i32 index, i32 axis);
        static bool IsGamepadButtonDown(i32 index, i32 button);
        
    private:
        friend class Window;
        
        static void ProcessKeyEvent(KeyCode key, bool pressed);
        static void ProcessMouseButtonEvent(MouseButton button, bool pressed);
        static void ProcessMouseMoveEvent(f32 x, f32 y);
        static void ProcessMouseScrollEvent(f32 delta);
    };
}
