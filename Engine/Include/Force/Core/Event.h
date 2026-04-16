#pragma once

#include "Force/Core/Base.h"
#include <functional>
#include <unordered_map>
#include <vector>
#include <typeindex>
#include <any>

namespace Force
{
    using EventID = u64;
    
    class Event
    {
    public:
        virtual ~Event() = default;
        virtual const char* GetName() const = 0;
        virtual EventID GetEventID() const = 0;
        
        bool Handled = false;
    };
    
    template<typename T>
    constexpr EventID GetEventTypeID()
    {
        return typeid(T).hash_code();
    }
    
    #define FORCE_EVENT_CLASS(type) \
        static EventID GetStaticEventID() { return GetEventTypeID<type>(); } \
        EventID GetEventID() const override { return GetStaticEventID(); } \
        const char* GetName() const override { return #type; }
    
    // Window Events
    class WindowCloseEvent : public Event
    {
    public:
        FORCE_EVENT_CLASS(WindowCloseEvent)
    };
    
    class WindowResizeEvent : public Event
    {
    public:
        WindowResizeEvent(u32 width, u32 height) : Width(width), Height(height) {}
        
        u32 Width;
        u32 Height;
        
        FORCE_EVENT_CLASS(WindowResizeEvent)
    };
    
    class WindowFocusEvent : public Event
    {
    public:
        WindowFocusEvent(bool focused) : Focused(focused) {}
        
        bool Focused;
        
        FORCE_EVENT_CLASS(WindowFocusEvent)
    };
    
    // Keyboard Events
    class KeyEvent : public Event
    {
    public:
        i32 KeyCode;
        i32 Mods;
        
    protected:
        KeyEvent(i32 keyCode, i32 mods) : KeyCode(keyCode), Mods(mods) {}
    };
    
    class KeyPressedEvent : public KeyEvent
    {
    public:
        KeyPressedEvent(i32 keyCode, i32 mods, bool repeat = false)
            : KeyEvent(keyCode, mods), Repeat(repeat) {}
        
        bool Repeat;
        
        FORCE_EVENT_CLASS(KeyPressedEvent)
    };
    
    class KeyReleasedEvent : public KeyEvent
    {
    public:
        KeyReleasedEvent(i32 keyCode, i32 mods) : KeyEvent(keyCode, mods) {}
        
        FORCE_EVENT_CLASS(KeyReleasedEvent)
    };
    
    class KeyTypedEvent : public Event
    {
    public:
        KeyTypedEvent(u32 codepoint) : Codepoint(codepoint) {}
        
        u32 Codepoint;
        
        FORCE_EVENT_CLASS(KeyTypedEvent)
    };
    
    // Mouse Events
    class MouseMovedEvent : public Event
    {
    public:
        MouseMovedEvent(f32 x, f32 y) : X(x), Y(y) {}
        
        f32 X, Y;
        
        FORCE_EVENT_CLASS(MouseMovedEvent)
    };
    
    class MouseScrolledEvent : public Event
    {
    public:
        MouseScrolledEvent(f32 xOffset, f32 yOffset) : XOffset(xOffset), YOffset(yOffset) {}
        
        f32 XOffset, YOffset;
        
        FORCE_EVENT_CLASS(MouseScrolledEvent)
    };
    
    class MouseButtonEvent : public Event
    {
    public:
        i32 Button;
        
    protected:
        MouseButtonEvent(i32 button) : Button(button) {}
    };
    
    class MouseButtonPressedEvent : public MouseButtonEvent
    {
    public:
        MouseButtonPressedEvent(i32 button) : MouseButtonEvent(button) {}
        
        FORCE_EVENT_CLASS(MouseButtonPressedEvent)
    };
    
    class MouseButtonReleasedEvent : public MouseButtonEvent
    {
    public:
        MouseButtonReleasedEvent(i32 button) : MouseButtonEvent(button) {}
        
        FORCE_EVENT_CLASS(MouseButtonReleasedEvent)
    };
    
    // Event Bus
    class EventBus
    {
    public:
        using EventCallback = std::function<void(Event&)>;
        using HandlerID = u64;
        
        static EventBus& Get();
        
        template<typename T>
        HandlerID Subscribe(std::function<void(T&)> callback)
        {
            static_assert(std::is_base_of_v<Event, T>, "T must derive from Event");
            
            EventID eventID = GetEventTypeID<T>();
            HandlerID handlerID = m_NextHandlerID++;
            
            m_Handlers[eventID].push_back({
                handlerID,
                [callback](Event& e) { callback(static_cast<T&>(e)); }
            });
            
            return handlerID;
        }
        
        void Unsubscribe(HandlerID handlerID);
        
        template<typename T, typename... Args>
        void Publish(Args&&... args)
        {
            T event(std::forward<Args>(args)...);
            Dispatch(event);
        }
        
        void Dispatch(Event& event);
        
        void ProcessQueue();
        
        template<typename T, typename... Args>
        void QueueEvent(Args&&... args)
        {
            m_EventQueue.push_back(std::make_any<T>(std::forward<Args>(args)...));
        }
        
    private:
        EventBus() = default;
        
        struct Handler
        {
            HandlerID ID;
            EventCallback Callback;
        };
        
        std::unordered_map<EventID, std::vector<Handler>> m_Handlers;
        std::vector<std::any> m_EventQueue;
        HandlerID m_NextHandlerID = 1;
    };
    
    // Event Dispatcher helper
    class EventDispatcher
    {
    public:
        EventDispatcher(Event& event) : m_Event(event) {}
        
        template<typename T, typename Func>
        bool Dispatch(Func&& func)
        {
            if (m_Event.GetEventID() == GetEventTypeID<T>())
            {
                m_Event.Handled |= func(static_cast<T&>(m_Event));
                return true;
            }
            return false;
        }
        
    private:
        Event& m_Event;
    };
}
