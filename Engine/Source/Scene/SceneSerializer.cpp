#include "Force/Scene/SceneSerializer.h"
#include "Force/Core/Logger.h"
#include <nlohmann/json.hpp>
#include <fstream>

namespace Force
{
    using json = nlohmann::json;

    SceneSerializer::SceneSerializer(Scene* scene)
        : m_Scene(scene)
    {
    }

    bool SceneSerializer::Serialize(const std::filesystem::path& filepath, SceneFormat format)
    {
        if (format == SceneFormat::Binary)
        {
            FORCE_CORE_WARN("Binary scene serialization not yet implemented, using JSON");
        }

        json j;
        j["scene"] = m_Scene->GetName();
        j["entities"] = json::array();

        // Serialize all entities
        auto view = m_Scene->GetAllEntitiesWith<TagComponent>();
        for (auto entity : view)
        {
            json entityJson;
            SerializeEntity(&entityJson, entity);
            j["entities"].push_back(entityJson);
        }

        try
        {
            std::ofstream file(filepath);
            if (!file.is_open())
            {
                FORCE_CORE_ERROR("SceneSerializer: Failed to open file: {}", filepath.string());
                return false;
            }

            file << j.dump(2);
            FORCE_CORE_INFO("SceneSerializer: Saved scene to: {}", filepath.string());
            return true;
        }
        catch (const std::exception& e)
        {
            FORCE_CORE_ERROR("SceneSerializer: Failed to serialize: {}", e.what());
            return false;
        }
    }

    bool SceneSerializer::Deserialize(const std::filesystem::path& filepath)
    {
        try
        {
            std::ifstream file(filepath);
            if (!file.is_open())
            {
                FORCE_CORE_ERROR("SceneSerializer: Failed to open file: {}", filepath.string());
                return false;
            }

            json j;
            file >> j;

            if (j.contains("scene"))
            {
                m_Scene->SetName(j["scene"].get<std::string>());
            }

            if (j.contains("entities"))
            {
                for (const auto& entityJson : j["entities"])
                {
                    DeserializeEntity(&entityJson);
                }
            }

            FORCE_CORE_INFO("SceneSerializer: Loaded scene from: {}", filepath.string());
            return true;
        }
        catch (const std::exception& e)
        {
            FORCE_CORE_ERROR("SceneSerializer: Failed to deserialize: {}", e.what());
            return false;
        }
    }

    void SceneSerializer::SerializeEntity(void* out, Entity entity)
    {
        json& j = *static_cast<json*>(out);

        auto& registry = m_Scene->GetRegistry();

        // UUID component (if exists)
        // For now, generate one on save if not present
        UUID uuid = UUID::Generate();
        j["uuid"] = uuid.ToString();

        // Tag component
        if (registry.all_of<TagComponent>(entity))
        {
            auto& tag = registry.get<TagComponent>(entity);
            j["tag"] = tag.Tag;
        }

        // Transform component
        if (registry.all_of<TransformComponent>(entity))
        {
            auto& transform = registry.get<TransformComponent>(entity);
            j["transform"]["position"] = { transform.Position.x, transform.Position.y, transform.Position.z };
            j["transform"]["rotation"] = { transform.Rotation.x, transform.Rotation.y, transform.Rotation.z };
            j["transform"]["scale"] = { transform.Scale.x, transform.Scale.y, transform.Scale.z };
        }

        // Relationship component
        if (registry.all_of<RelationshipComponent>(entity))
        {
            auto& rel = registry.get<RelationshipComponent>(entity);
            j["relationship"]["parent"] = rel.Parent != NullEntity;
            // Children serialized separately
        }

        // Active component
        if (registry.all_of<ActiveComponent>(entity))
        {
            auto& active = registry.get<ActiveComponent>(entity);
            j["active"] = active.Active;
        }

        // Serialize other components
        SerializeComponents(&j, entity);
    }

    Entity SceneSerializer::DeserializeEntity(const void* in)
    {
        const json& j = *static_cast<const json*>(in);

        std::string name = "Entity";
        if (j.contains("tag"))
        {
            name = j["tag"].get<std::string>();
        }

        UUID uuid;
        if (j.contains("uuid"))
        {
            uuid = UUID::FromString(j["uuid"].get<std::string>());
        }
        else
        {
            uuid = UUID::Generate();
        }

        EntityHandle entityHandle = m_Scene->CreateEntityWithUUID(uuid, name);
        Entity entity = entityHandle.GetEntity();

        auto& registry = m_Scene->GetRegistry();

        // Transform component
        if (j.contains("transform"))
        {
            auto& t = j["transform"];
            TransformComponent transform;

            if (t.contains("position"))
            {
                transform.Position = glm::vec3(
                    t["position"][0].get<float>(),
                    t["position"][1].get<float>(),
                    t["position"][2].get<float>()
                );
            }
            if (t.contains("rotation"))
            {
                transform.Rotation = glm::vec3(
                    t["rotation"][0].get<float>(),
                    t["rotation"][1].get<float>(),
                    t["rotation"][2].get<float>()
                );
            }
            if (t.contains("scale"))
            {
                transform.Scale = glm::vec3(
                    t["scale"][0].get<float>(),
                    t["scale"][1].get<float>(),
                    t["scale"][2].get<float>()
                );
            }

            registry.emplace_or_replace<TransformComponent>(entity, transform);
        }

        // Active component
        if (j.contains("active"))
        {
            ActiveComponent active;
            active.Active = j["active"].get<bool>();
            registry.emplace_or_replace<ActiveComponent>(entity, active);
        }

        // Deserialize other components
        DeserializeComponents(&j, entity);

        return entity;
    }

    void SceneSerializer::SerializeComponents(void* out, Entity entity)
    {
        json& j = *static_cast<json*>(out);
        auto& registry = m_Scene->GetRegistry();

        // Add serialization for custom components here
        // Example:
        // if (registry.all_of<MeshComponent>(entity))
        // {
        //     auto& mesh = registry.get<MeshComponent>(entity);
        //     j["meshComponent"]["assetHandle"] = mesh.MeshHandle.ToString();
        // }
    }

    void SceneSerializer::DeserializeComponents(const void* in, Entity entity)
    {
        const json& j = *static_cast<const json*>(in);
        auto& registry = m_Scene->GetRegistry();

        // Add deserialization for custom components here
    }

    std::string SceneSerializer::SerializeEntity(Entity entity)
    {
        json j;
        SerializeEntity(&j, entity);
        return j.dump();
    }

    Entity SceneSerializer::DeserializeEntity(const std::string& data)
    {
        json j = json::parse(data);
        return DeserializeEntity(&j);
    }
}
