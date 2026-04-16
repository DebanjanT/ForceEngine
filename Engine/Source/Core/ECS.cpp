#include "Force/Core/ECS.h"
#include "Force/Core/Logger.h"

namespace Force
{
    Scene::Scene(const std::string& name)
        : m_Name(name)
    {
    }
    
    Scene::~Scene()
    {
        m_Registry.clear();
    }
    
    EntityHandle Scene::CreateEntity(const std::string& name)
    {
        return CreateEntityWithUUID(UUID(), name);
    }
    
    EntityHandle Scene::CreateEntityWithUUID(UUID uuid, const std::string& name)
    {
        Entity entity = m_Registry.create();
        
        m_Registry.emplace<TagComponent>(entity, name);
        m_Registry.emplace<TransformComponent>(entity);
        m_Registry.emplace<ActiveComponent>(entity);
        
        m_EntityMap[uuid] = entity;
        
        return EntityHandle(entity, this);
    }
    
    void Scene::DestroyEntity(EntityHandle entity)
    {
        // Remove from entity map
        for (auto it = m_EntityMap.begin(); it != m_EntityMap.end(); ++it)
        {
            if (it->second == entity.GetEntity())
            {
                m_EntityMap.erase(it);
                break;
            }
        }
        
        m_Registry.destroy(entity.GetEntity());
    }
    
    EntityHandle Scene::FindEntityByName(const std::string& name)
    {
        auto view = m_Registry.view<TagComponent>();
        for (auto entity : view)
        {
            const auto& tag = view.get<TagComponent>(entity);
            if (tag.Tag == name)
            {
                return EntityHandle(entity, this);
            }
        }
        return EntityHandle();
    }
    
    EntityHandle Scene::FindEntityByUUID(UUID uuid)
    {
        auto it = m_EntityMap.find(uuid);
        if (it != m_EntityMap.end())
        {
            return EntityHandle(it->second, this);
        }
        return EntityHandle();
    }
    
    void Scene::OnUpdate(f32 deltaTime)
    {
        // Update systems here
    }
    
    void Scene::OnRender()
    {
        // Render systems here
    }
}
