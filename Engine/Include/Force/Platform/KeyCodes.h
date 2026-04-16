#pragma once

#include "Force/Core/Types.h"

namespace Force
{
    enum class KeyCode : u16
    {
        // Unknown
        Unknown = 0,
        
        // Printable keys
        Space = 32,
        Apostrophe = 39,
        Comma = 44,
        Minus = 45,
        Period = 46,
        Slash = 47,
        
        // Numbers
        D0 = 48, D1, D2, D3, D4, D5, D6, D7, D8, D9,
        
        Semicolon = 59,
        Equal = 61,
        
        // Letters
        A = 65, B, C, D, E, F, G, H, I, J, K, L, M,
        N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
        
        LeftBracket = 91,
        Backslash = 92,
        RightBracket = 93,
        GraveAccent = 96,
        
        // Function keys
        Escape = 256,
        Enter = 257,
        Tab = 258,
        Backspace = 259,
        Insert = 260,
        Delete = 261,
        Right = 262,
        Left = 263,
        Down = 264,
        Up = 265,
        PageUp = 266,
        PageDown = 267,
        Home = 268,
        End = 269,
        CapsLock = 280,
        ScrollLock = 281,
        NumLock = 282,
        PrintScreen = 283,
        Pause = 284,
        
        F1 = 290, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
        F13, F14, F15, F16, F17, F18, F19, F20, F21, F22, F23, F24, F25,
        
        // Keypad
        KP0 = 320, KP1, KP2, KP3, KP4, KP5, KP6, KP7, KP8, KP9,
        KPDecimal = 330,
        KPDivide = 331,
        KPMultiply = 332,
        KPSubtract = 333,
        KPAdd = 334,
        KPEnter = 335,
        KPEqual = 336,
        
        // Modifiers
        LeftShift = 340,
        LeftControl = 341,
        LeftAlt = 342,
        LeftSuper = 343,
        RightShift = 344,
        RightControl = 345,
        RightAlt = 346,
        RightSuper = 347,
        Menu = 348,
        
        MaxKeys
    };
    
    enum class MouseButton : u8
    {
        Left = 0,
        Right = 1,
        Middle = 2,
        Button4 = 3,
        Button5 = 4,
        Button6 = 5,
        Button7 = 6,
        Button8 = 7,
        
        MaxButtons
    };
    
    enum class KeyMod : u8
    {
        None = 0,
        Shift = 1 << 0,
        Control = 1 << 1,
        Alt = 1 << 2,
        Super = 1 << 3,
        CapsLock = 1 << 4,
        NumLock = 1 << 5
    };
    
    inline KeyMod operator|(KeyMod a, KeyMod b)
    {
        return static_cast<KeyMod>(static_cast<u8>(a) | static_cast<u8>(b));
    }
    
    inline KeyMod operator&(KeyMod a, KeyMod b)
    {
        return static_cast<KeyMod>(static_cast<u8>(a) & static_cast<u8>(b));
    }
}
