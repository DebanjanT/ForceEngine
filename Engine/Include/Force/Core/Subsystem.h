#pragma once

#include "Force/Core/Base.h"
#include <typeindex>
#include <unordered_map>
#include <vector>
#include <functional>

namespace Force
{
    class Subsystem
    {
    public:
        virtual ~Subsystem() = default;
        
        virtual void Init() {}
        virtual void Shutdown() {}
        virtual void Update(f32 deltaTime) {}
        
        virtual const char* GetName() const = 0;
        virtual i32 GetPriority() const { return 0; }
        
        bool IsInitialized() const { return m_Initialized; }
        
    protected:
        friend class SubsystemRegistry;
        bool m_Initialized = false;
    };
    
    class SubsystemRegistry
    {
    public:
        static SubsystemRegistry& Get();
        
        template<typename T, typename... Args>
        T* Register(Args&&... args)
        {
            static_assert(std::is_base_of_v<Subsystem, T>, "T must derive from Subsystem");
            
            auto subsystem = CreateScope<T>(std::forward<Args>(args)...);
            T* ptr = subsystem.get();
            
            std::type_index typeIndex(typeid(T));
            m_Subsystems[typeIndex] = std::move(subsystem);
            m_OrderedSubsystems.push_back(ptr);
            
            SortByPriority();
            
            return ptr;
        }
        
        template<typename T>
        T* Get()
        {
            std::type_index typeIndex(typeid(T));
            auto it = m_Subsystems.find(typeIndex);
            if (it != m_Subsystems.end())
            {
                return static_cast<T*>(it->second.get());
            }
            return nullptr;
        }
        
        template<typename T>
        void Unregister()
        {
            std::type_index typeIndex(typeid(T));
            auto it = m_Subsystems.find(typeIndex);
            if (it != m_Subsystems.end())
            {
                Subsystem* ptr = it->second.get();
                
                auto orderIt = std::find(m_OrderedSubsystems.begin(), m_OrderedSubsystems.end(), ptr);
                if (orderIt != m_OrderedSubsystems.end())
                {
                    m_OrderedSubsystems.erase(orderIt);
                }
                
                if (ptr->m_Initialized)
                {
                    ptr->Shutdown();
                }
                
                m_Subsystems.erase(it);
            }
        }
        
        void InitAll();
        void ShutdownAll();
        void UpdateAll(f32 deltaTime);
        
        template<typename Func>
        void ForEach(Func&& func)
        {
            for (auto* subsystem : m_OrderedSubsystems)
            {
                func(subsystem);
            }
        }
        
    private:
        SubsystemRegistry() = default;
        ~SubsystemRegistry() = default;
        
        void SortByPriority();
        
        std::unordered_map<std::type_index, Scope<Subsystem>> m_Subsystems;
        std::vector<Subsystem*> m_OrderedSubsystems;
    };
}
