#include "Force/Platform/Input.h"
#include <cstring>

namespace Force
{
    static bool s_KeyStates[static_cast<size_t>(KeyCode::MaxKeys)] = {};
    static bool s_PrevKeyStates[static_cast<size_t>(KeyCode::MaxKeys)] = {};
    
    static bool s_MouseButtonStates[static_cast<size_t>(MouseButton::MaxButtons)] = {};
    static bool s_PrevMouseButtonStates[static_cast<size_t>(MouseButton::MaxButtons)] = {};
    
    static glm::vec2 s_MousePosition = { 0.0f, 0.0f };
    static glm::vec2 s_PrevMousePosition = { 0.0f, 0.0f };
    static f32 s_MouseScrollDelta = 0.0f;
    
    void Input::Init()
    {
        std::memset(s_KeyStates, 0, sizeof(s_KeyStates));
        std::memset(s_PrevKeyStates, 0, sizeof(s_PrevKeyStates));
        std::memset(s_MouseButtonStates, 0, sizeof(s_MouseButtonStates));
        std::memset(s_PrevMouseButtonStates, 0, sizeof(s_PrevMouseButtonStates));
    }
    
    void Input::Update()
    {
        std::memcpy(s_PrevKeyStates, s_KeyStates, sizeof(s_KeyStates));
        std::memcpy(s_PrevMouseButtonStates, s_MouseButtonStates, sizeof(s_MouseButtonStates));
        s_PrevMousePosition = s_MousePosition;
        s_MouseScrollDelta = 0.0f;
    }
    
    bool Input::IsKeyDown(KeyCode key)
    {
        return s_KeyStates[static_cast<size_t>(key)];
    }
    
    bool Input::IsKeyUp(KeyCode key)
    {
        return !s_KeyStates[static_cast<size_t>(key)];
    }
    
    bool Input::WasKeyPressed(KeyCode key)
    {
        size_t index = static_cast<size_t>(key);
        return s_KeyStates[index] && !s_PrevKeyStates[index];
    }
    
    bool Input::WasKeyReleased(KeyCode key)
    {
        size_t index = static_cast<size_t>(key);
        return !s_KeyStates[index] && s_PrevKeyStates[index];
    }
    
    bool Input::IsMouseButtonDown(MouseButton button)
    {
        return s_MouseButtonStates[static_cast<size_t>(button)];
    }
    
    bool Input::IsMouseButtonUp(MouseButton button)
    {
        return !s_MouseButtonStates[static_cast<size_t>(button)];
    }
    
    bool Input::WasMouseButtonPressed(MouseButton button)
    {
        size_t index = static_cast<size_t>(button);
        return s_MouseButtonStates[index] && !s_PrevMouseButtonStates[index];
    }
    
    bool Input::WasMouseButtonReleased(MouseButton button)
    {
        size_t index = static_cast<size_t>(button);
        return !s_MouseButtonStates[index] && s_PrevMouseButtonStates[index];
    }
    
    glm::vec2 Input::GetMousePosition()
    {
        return s_MousePosition;
    }
    
    glm::vec2 Input::GetMouseDelta()
    {
        return s_MousePosition - s_PrevMousePosition;
    }
    
    f32 Input::GetMouseScrollDelta()
    {
        return s_MouseScrollDelta;
    }
    
    bool Input::IsGamepadConnected(i32 index)
    {
        return false; // TODO: Implement gamepad support
    }
    
    f32 Input::GetGamepadAxis(i32 index, i32 axis)
    {
        return 0.0f; // TODO: Implement gamepad support
    }
    
    bool Input::IsGamepadButtonDown(i32 index, i32 button)
    {
        return false; // TODO: Implement gamepad support
    }
    
    void Input::ProcessKeyEvent(KeyCode key, bool pressed)
    {
        s_KeyStates[static_cast<size_t>(key)] = pressed;
    }
    
    void Input::ProcessMouseButtonEvent(MouseButton button, bool pressed)
    {
        s_MouseButtonStates[static_cast<size_t>(button)] = pressed;
    }
    
    void Input::ProcessMouseMoveEvent(f32 x, f32 y)
    {
        s_MousePosition = { x, y };
    }
    
    void Input::ProcessMouseScrollEvent(f32 delta)
    {
        s_MouseScrollDelta = delta;
    }
}
