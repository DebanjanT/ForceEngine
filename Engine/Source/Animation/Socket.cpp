#include "Force/Animation/Socket.h"
#include "Force/Scene/Components.h"
#include "Force/Core/Logger.h"
#include <glm/gtc/matrix_transform.hpp>

namespace Force
{
    void SocketSystem::UpdateSocketTransforms(Scene* scene)
    {
        auto& registry = scene->GetRegistry();

        // Get all entities with socket components
        auto view = registry.view<TransformComponent, SocketComponent>();

        for (auto entity : view)
        {
            auto& transform = view.get<TransformComponent>(entity);
            auto& sockets = view.get<SocketComponent>(entity);

            glm::mat4 entityWorldTransform = transform.GetTransform();

            // For skeletal meshes, we would get bone transforms here
            // For now, just use the entity transform as the bone transform
            // TODO: Integrate with skeleton/animation system

            for (auto& socket : sockets.Sockets)
            {
                // Get bone transform (placeholder - use entity transform for now)
                glm::mat4 boneTransform = entityWorldTransform;

                // TODO: Look up actual bone transform from skeleton
                // glm::mat4 boneTransform = skeleton->GetBoneWorldTransform(socket.BoneName);

                // Compute socket world transform
                socket.WorldTransform = boneTransform * socket.GetRelativeTransform();
            }
        }
    }

    void SocketSystem::UpdateAttachedEntities(Scene* scene)
    {
        auto& registry = scene->GetRegistry();

        auto view = registry.view<TransformComponent, AttachedToSocketComponent>();

        for (auto entity : view)
        {
            auto& transform = view.get<TransformComponent>(entity);
            auto& attachment = view.get<AttachedToSocketComponent>(entity);

            if (attachment.ParentEntity == NullEntity)
                continue;

            // Get the socket world transform
            glm::mat4 socketTransform = GetSocketWorldTransform(scene, attachment.ParentEntity, attachment.SocketName);

            if (socketTransform == glm::mat4(0.0f))
                continue; // Socket not found

            // Apply offset
            glm::mat4 offsetTransform = glm::translate(glm::mat4(1.0f), attachment.PositionOffset);
            offsetTransform *= glm::mat4_cast(attachment.RotationOffset);

            glm::mat4 finalTransform = socketTransform * offsetTransform;

            // Extract position, rotation, scale from final transform
            if (attachment.InheritPosition)
            {
                transform.Position = glm::vec3(finalTransform[3]);
            }

            if (attachment.InheritRotation)
            {
                // Extract rotation from matrix
                glm::vec3 scale;
                scale.x = glm::length(glm::vec3(finalTransform[0]));
                scale.y = glm::length(glm::vec3(finalTransform[1]));
                scale.z = glm::length(glm::vec3(finalTransform[2]));

                glm::mat3 rotMatrix(
                    glm::vec3(finalTransform[0]) / scale.x,
                    glm::vec3(finalTransform[1]) / scale.y,
                    glm::vec3(finalTransform[2]) / scale.z
                );

                glm::quat rotation = glm::quat_cast(rotMatrix);

                // Convert to Euler angles
                transform.Rotation = glm::eulerAngles(rotation);
            }

            if (attachment.InheritScale)
            {
                transform.Scale.x = glm::length(glm::vec3(finalTransform[0]));
                transform.Scale.y = glm::length(glm::vec3(finalTransform[1]));
                transform.Scale.z = glm::length(glm::vec3(finalTransform[2]));
            }
        }
    }

    void SocketSystem::AttachToSocket(Scene* scene, Entity entity, Entity parent, const std::string& socketName)
    {
        auto& registry = scene->GetRegistry();

        // Verify parent has socket component
        if (!registry.all_of<SocketComponent>(parent))
        {
            FORCE_CORE_ERROR("SocketSystem: Parent entity does not have SocketComponent");
            return;
        }

        auto& sockets = registry.get<SocketComponent>(parent);
        if (!sockets.HasSocket(socketName))
        {
            FORCE_CORE_ERROR("SocketSystem: Socket '{}' not found on parent entity", socketName);
            return;
        }

        // Add or update attachment component
        auto& attachment = registry.get_or_emplace<AttachedToSocketComponent>(entity);
        attachment.ParentEntity = parent;
        attachment.SocketName = socketName;

        FORCE_CORE_INFO("SocketSystem: Attached entity to socket '{}'", socketName);
    }

    void SocketSystem::DetachFromSocket(Scene* scene, Entity entity)
    {
        auto& registry = scene->GetRegistry();

        if (registry.all_of<AttachedToSocketComponent>(entity))
        {
            registry.remove<AttachedToSocketComponent>(entity);
            FORCE_CORE_INFO("SocketSystem: Detached entity from socket");
        }
    }

    glm::mat4 SocketSystem::GetSocketWorldTransform(Scene* scene, Entity parentEntity, const std::string& socketName)
    {
        auto& registry = scene->GetRegistry();

        if (!registry.all_of<SocketComponent>(parentEntity))
        {
            return glm::mat4(0.0f); // Invalid
        }

        auto& sockets = registry.get<SocketComponent>(parentEntity);
        const Socket* socket = sockets.GetSocket(socketName);

        if (!socket)
        {
            return glm::mat4(0.0f); // Invalid
        }

        return socket->WorldTransform;
    }
}
