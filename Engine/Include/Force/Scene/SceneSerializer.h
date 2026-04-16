#pragma once

#include "Force/Core/Base.h"
#include "Force/Core/ECS.h"
#include <filesystem>

namespace Force
{
    // Scene serialization formats
    enum class SceneFormat : u8
    {
        JSON,     // Human-readable
        Binary    // Fast loading
    };

    class SceneSerializer
    {
    public:
        SceneSerializer(Scene* scene);

        // Serialize to file
        bool Serialize(const std::filesystem::path& filepath, SceneFormat format = SceneFormat::JSON);

        // Deserialize from file
        bool Deserialize(const std::filesystem::path& filepath);

        // Serialize/Deserialize single entity (for prefabs)
        std::string SerializeEntity(Entity entity);
        Entity DeserializeEntity(const std::string& data);

    private:
        void SerializeEntity(void* out, Entity entity);  // nlohmann::json*
        Entity DeserializeEntity(const void* in);        // nlohmann::json*

        void SerializeComponents(void* out, Entity entity);
        void DeserializeComponents(const void* in, Entity entity);

    private:
        Scene* m_Scene;
    };
}
