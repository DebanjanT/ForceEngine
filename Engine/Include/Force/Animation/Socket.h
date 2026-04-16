#pragma once

#include "Force/Core/Base.h"
#include "Force/Core/ECS.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <vector>
#include <unordered_map>

namespace Force
{
    // A socket is a named attachment point on a skeleton
    struct Socket
    {
        std::string Name;
        std::string BoneName;           // Parent bone this socket is attached to
        glm::vec3   RelativePosition = glm::vec3(0.0f);
        glm::quat   RelativeRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        glm::vec3   RelativeScale = glm::vec3(1.0f);

        // Computed world transform (updated each frame)
        glm::mat4   WorldTransform = glm::mat4(1.0f);

        glm::mat4 GetRelativeTransform() const
        {
            glm::mat4 transform = glm::translate(glm::mat4(1.0f), RelativePosition);
            transform *= glm::mat4_cast(RelativeRotation);
            transform = glm::scale(transform, RelativeScale);
            return transform;
        }
    };

    // Component that holds socket definitions for a skeletal mesh
    struct SocketComponent
    {
        std::vector<Socket> Sockets;
        std::unordered_map<std::string, u32> SocketNameToIndex;

        void AddSocket(const Socket& socket)
        {
            SocketNameToIndex[socket.Name] = static_cast<u32>(Sockets.size());
            Sockets.push_back(socket);
        }

        Socket* GetSocket(const std::string& name)
        {
            auto it = SocketNameToIndex.find(name);
            if (it != SocketNameToIndex.end())
            {
                return &Sockets[it->second];
            }
            return nullptr;
        }

        const Socket* GetSocket(const std::string& name) const
        {
            auto it = SocketNameToIndex.find(name);
            if (it != SocketNameToIndex.end())
            {
                return &Sockets[it->second];
            }
            return nullptr;
        }

        bool HasSocket(const std::string& name) const
        {
            return SocketNameToIndex.find(name) != SocketNameToIndex.end();
        }

        void RemoveSocket(const std::string& name)
        {
            auto it = SocketNameToIndex.find(name);
            if (it != SocketNameToIndex.end())
            {
                u32 index = it->second;
                Sockets.erase(Sockets.begin() + index);
                SocketNameToIndex.erase(it);

                // Update indices for sockets after the removed one
                for (auto& [n, idx] : SocketNameToIndex)
                {
                    if (idx > index)
                    {
                        idx--;
                    }
                }
            }
        }
    };

    // Component for entities that are attached to a socket
    struct AttachedToSocketComponent
    {
        Entity ParentEntity = NullEntity;       // Entity with SocketComponent
        std::string SocketName;                  // Name of the socket to attach to
        bool InheritPosition = true;
        bool InheritRotation = true;
        bool InheritScale = false;

        // Offset from socket transform
        glm::vec3 PositionOffset = glm::vec3(0.0f);
        glm::quat RotationOffset = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    };

    // System to update socket transforms
    class SocketSystem
    {
    public:
        // Update all socket world transforms based on skeleton pose
        static void UpdateSocketTransforms(Scene* scene);

        // Update transforms of entities attached to sockets
        static void UpdateAttachedEntities(Scene* scene);

        // Attach an entity to a socket
        static void AttachToSocket(Scene* scene, Entity entity, Entity parent, const std::string& socketName);

        // Detach entity from socket
        static void DetachFromSocket(Scene* scene, Entity entity);

        // Get the current world transform of a socket
        static glm::mat4 GetSocketWorldTransform(Scene* scene, Entity parentEntity, const std::string& socketName);
    };
}
