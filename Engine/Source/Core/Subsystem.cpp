#include "Force/Core/Subsystem.h"
#include "Force/Core/Logger.h"
#include <algorithm>

namespace Force
{
    SubsystemRegistry& SubsystemRegistry::Get()
    {
        static SubsystemRegistry instance;
        return instance;
    }
    
    void SubsystemRegistry::InitAll()
    {
        FORCE_CORE_INFO("Initializing {} subsystems...", m_OrderedSubsystems.size());
        
        for (auto* subsystem : m_OrderedSubsystems)
        {
            if (!subsystem->m_Initialized)
            {
                FORCE_CORE_TRACE("  Initializing: {}", subsystem->GetName());
                subsystem->Init();
                subsystem->m_Initialized = true;
            }
        }
    }
    
    void SubsystemRegistry::ShutdownAll()
    {
        FORCE_CORE_INFO("Shutting down {} subsystems...", m_OrderedSubsystems.size());
        
        for (auto it = m_OrderedSubsystems.rbegin(); it != m_OrderedSubsystems.rend(); ++it)
        {
            auto* subsystem = *it;
            if (subsystem->m_Initialized)
            {
                FORCE_CORE_TRACE("  Shutting down: {}", subsystem->GetName());
                subsystem->Shutdown();
                subsystem->m_Initialized = false;
            }
        }
        
        m_OrderedSubsystems.clear();
        m_Subsystems.clear();
    }
    
    void SubsystemRegistry::UpdateAll(f32 deltaTime)
    {
        for (auto* subsystem : m_OrderedSubsystems)
        {
            if (subsystem->m_Initialized)
            {
                subsystem->Update(deltaTime);
            }
        }
    }
    
    void SubsystemRegistry::SortByPriority()
    {
        std::sort(m_OrderedSubsystems.begin(), m_OrderedSubsystems.end(),
            [](const Subsystem* a, const Subsystem* b)
            {
                return a->GetPriority() < b->GetPriority();
            });
    }
}
