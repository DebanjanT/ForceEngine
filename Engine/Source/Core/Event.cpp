#include "Force/Core/Event.h"

namespace Force
{
    EventBus& EventBus::Get()
    {
        static EventBus instance;
        return instance;
    }
    
    void EventBus::Unsubscribe(HandlerID handlerID)
    {
        for (auto& [eventID, handlers] : m_Handlers)
        {
            auto it = std::remove_if(handlers.begin(), handlers.end(),
                [handlerID](const Handler& h) { return h.ID == handlerID; });
            handlers.erase(it, handlers.end());
        }
    }
    
    void EventBus::Dispatch(Event& event)
    {
        EventID eventID = event.GetEventID();
        
        auto it = m_Handlers.find(eventID);
        if (it != m_Handlers.end())
        {
            for (auto& handler : it->second)
            {
                if (!event.Handled)
                {
                    handler.Callback(event);
                }
            }
        }
    }
    
    void EventBus::ProcessQueue()
    {
        // Process queued events - simplified implementation
        // In a full implementation, you'd need type erasure to dispatch properly
        m_EventQueue.clear();
    }
}
