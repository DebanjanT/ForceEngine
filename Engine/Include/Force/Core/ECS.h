#pragma once

#include "Force/Core/Base.h"
#include "Force/Math/Math.h"
#include <entt/entt.hpp>

namespace Force
{
    using Entity = entt::entity;
    using Registry = entt::registry;
    
    constexpr Entity NullEntity = entt::null;
    
    class Scene;
    
    class EntityHandle
    {
    public:
        EntityHandle() = default;
        EntityHandle(Entity entity, Scene* scene) : m_Entity(entity), m_Scene(scene) {}
        
        operator bool() const { return m_Entity != NullEntity && m_Scene != nullptr; }
        operator Entity() const { return m_Entity; }
        
        bool operator==(const EntityHandle& other) const
        {
            return m_Entity == other.m_Entity && m_Scene == other.m_Scene;
        }
        
        bool operator!=(const EntityHandle& other) const
        {
            return !(*this == other);
        }
        
        template<typename T, typename... Args>
        T& AddComponent(Args&&... args);
        
        template<typename T>
        T& GetComponent();
        
        template<typename T>
        const T& GetComponent() const;
        
        template<typename T>
        bool HasComponent() const;
        
        template<typename T>
        void RemoveComponent();
        
        Entity GetEntity() const { return m_Entity; }
        Scene* GetScene() const { return m_Scene; }
        
    private:
        Entity m_Entity = NullEntity;
        Scene* m_Scene = nullptr;
    };
    
    // Common Components
    struct TagComponent
    {
        std::string Tag;
        
        TagComponent() = default;
        TagComponent(const std::string& tag) : Tag(tag) {}
    };
    
    struct TransformComponent
    {
        glm::vec3 Position = { 0.0f, 0.0f, 0.0f };
        glm::vec3 Rotation = { 0.0f, 0.0f, 0.0f }; // Euler angles in radians
        glm::vec3 Scale = { 1.0f, 1.0f, 1.0f };
        
        TransformComponent() = default;
        TransformComponent(const glm::vec3& position) : Position(position) {}
     
        glm::mat4 GetTransform() const
        {
            glm::mat4 rotation = glm::toMat4(glm::quat(Rotation));
            return glm::translate(glm::mat4(1.0f), Position)
                 * rotation
                 * glm::scale(glm::mat4(1.0f), Scale);
        }
    };
    
    struct RelationshipComponent
    {
        Entity Parent = NullEntity;
        std::vector<Entity> Children;
    };
    
    struct ActiveComponent
    {
        bool Active = true;
    };
    
    // Scene class
    class Scene
    {
    public:
        Scene(const std::string& name = "Untitled");
        ~Scene();
        
        EntityHandle CreateEntity(const std::string& name = "Entity");
        EntityHandle CreateEntityWithUUID(UUID uuid, const std::string& name = "Entity");
        void DestroyEntity(EntityHandle entity);
        
        EntityHandle FindEntityByName(const std::string& name);
        EntityHandle FindEntityByUUID(UUID uuid);
        
        void OnUpdate(f32 deltaTime);
        void OnRender();
        
        template<typename... Components>
        auto GetAllEntitiesWith()
        {
            return m_Registry.view<Components...>();
        }
        
        Registry& GetRegistry() { return m_Registry; }
        const std::string& GetName() const { return m_Name; }
        void SetName(const std::string& name) { m_Name = name; }
        
    private:
        std::string m_Name;
        Registry m_Registry;
        std::unordered_map<UUID, Entity> m_EntityMap;
    };
    
    // Template implementations
    template<typename T, typename... Args>
    T& EntityHandle::AddComponent(Args&&... args)
    {
        FORCE_CORE_ASSERT(!HasComponent<T>(), "Entity already has component!");
        return m_Scene->GetRegistry().emplace<T>(m_Entity, std::forward<Args>(args)...);
    }
    
    template<typename T>
    T& EntityHandle::GetComponent()
    {
        FORCE_CORE_ASSERT(HasComponent<T>(), "Entity does not have component!");
        return m_Scene->GetRegistry().get<T>(m_Entity);
    }
    
    template<typename T>
    const T& EntityHandle::GetComponent() const
    {
        FORCE_CORE_ASSERT(HasComponent<T>(), "Entity does not have component!");
        return m_Scene->GetRegistry().get<T>(m_Entity);
    }
    
    template<typename T>
    bool EntityHandle::HasComponent() const
    {
        return m_Scene->GetRegistry().all_of<T>(m_Entity);
    }
    
    template<typename T>
    void EntityHandle::RemoveComponent()
    {
        FORCE_CORE_ASSERT(HasComponent<T>(), "Entity does not have component!");
        m_Scene->GetRegistry().remove<T>(m_Entity);
    }
}
