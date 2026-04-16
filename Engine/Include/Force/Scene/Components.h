#pragma once

#include "Force/Core/Base.h"
#include "Force/Core/ECS.h"
#include "Force/Asset/AssetTypes.h"
#include "Force/Renderer/Material.h"
#include <glm/glm.hpp>
#include <vector>

namespace Force
{
    // UUID component for persistent entity identification
    struct UUIDComponent
    {
        UUID ID;

        UUIDComponent() = default;
        UUIDComponent(UUID uuid) : ID(uuid) {}
    };

    // Static mesh rendering component
    struct StaticMeshComponent
    {
        AssetHandle MeshHandle = NullAssetHandle;
        std::vector<Ref<Material>> Materials;

        bool Visible = true;
        bool CastShadow = true;
        bool ReceiveShadow = true;

        u32 CurrentLOD = 0;  // for manual LOD override, 0 = auto

        StaticMeshComponent() = default;
        StaticMeshComponent(AssetHandle meshHandle) : MeshHandle(meshHandle) {}
    };

    // Skeletal mesh rendering component
    struct SkeletalMeshComponent
    {
        AssetHandle MeshHandle = NullAssetHandle;
        AssetHandle SkeletonHandle = NullAssetHandle;
        std::vector<Ref<Material>> Materials;

        bool Visible = true;
        bool CastShadow = true;
        bool ReceiveShadow = true;

        // Current animation state
        AssetHandle CurrentAnimation = NullAssetHandle;
        f32 AnimationTime = 0.0f;
        f32 AnimationSpeed = 1.0f;
        bool AnimationLooping = true;

        SkeletalMeshComponent() = default;
    };

    // Camera component
    struct CameraComponent
    {
        enum class ProjectionType { Perspective, Orthographic };

        ProjectionType Projection = ProjectionType::Perspective;
        f32 FOV = 60.0f;            // degrees (perspective)
        f32 OrthoSize = 10.0f;      // orthographic size
        f32 NearClip = 0.1f;
        f32 FarClip = 1000.0f;

        bool Primary = false;       // Is this the main camera?
        bool FixedAspectRatio = false;
        f32 AspectRatio = 16.0f / 9.0f;

        glm::mat4 GetProjectionMatrix(f32 aspectRatio) const
        {
            f32 ar = FixedAspectRatio ? AspectRatio : aspectRatio;
            if (Projection == ProjectionType::Perspective)
            {
                return glm::perspective(glm::radians(FOV), ar, NearClip, FarClip);
            }
            else
            {
                f32 width = OrthoSize * ar;
                f32 height = OrthoSize;
                return glm::ortho(-width * 0.5f, width * 0.5f, -height * 0.5f, height * 0.5f, NearClip, FarClip);
            }
        }
    };

    // Light components
    struct DirectionalLightComponent
    {
        glm::vec3 Color = glm::vec3(1.0f);
        f32 Intensity = 1.0f;
        bool CastShadows = true;
    };

    struct PointLightComponent
    {
        glm::vec3 Color = glm::vec3(1.0f);
        f32 Intensity = 1.0f;
        f32 Radius = 10.0f;
        f32 FalloffExponent = 2.0f;
        bool CastShadows = false;
    };

    struct SpotLightComponent
    {
        glm::vec3 Color = glm::vec3(1.0f);
        f32 Intensity = 1.0f;
        f32 Range = 10.0f;
        f32 InnerConeAngle = 30.0f;  // degrees
        f32 OuterConeAngle = 45.0f;  // degrees
        bool CastShadows = false;
    };

    // Bounding volume component (auto-computed from mesh)
    struct BoundsComponent
    {
        glm::vec3 LocalMin = glm::vec3(0.0f);
        glm::vec3 LocalMax = glm::vec3(0.0f);
        glm::vec3 WorldMin = glm::vec3(0.0f);
        glm::vec3 WorldMax = glm::vec3(0.0f);

        glm::vec3 GetLocalCenter() const { return (LocalMin + LocalMax) * 0.5f; }
        glm::vec3 GetLocalExtents() const { return (LocalMax - LocalMin) * 0.5f; }
        glm::vec3 GetWorldCenter() const { return (WorldMin + WorldMax) * 0.5f; }
        glm::vec3 GetWorldExtents() const { return (WorldMax - WorldMin) * 0.5f; }
    };

    // Script component (for future scripting system)
    struct ScriptComponent
    {
        std::string ClassName;
        // TODO: Add script instance pointer
    };

    // Physics rigid body component
    struct RigidBodyComponent
    {
        enum class BodyType { Static, Kinematic, Dynamic };

        BodyType Type = BodyType::Dynamic;
        f32 Mass = 1.0f;
        f32 LinearDamping = 0.01f;
        f32 AngularDamping = 0.05f;
        bool UseGravity = true;
        bool IsKinematic = false;

        // Constraints
        bool FreezePositionX = false;
        bool FreezePositionY = false;
        bool FreezePositionZ = false;
        bool FreezeRotationX = false;
        bool FreezeRotationY = false;
        bool FreezeRotationZ = false;
    };

    // Box collider component
    struct BoxColliderComponent
    {
        glm::vec3 Size = glm::vec3(1.0f);
        glm::vec3 Offset = glm::vec3(0.0f);
        bool IsTrigger = false;
    };

    // Sphere collider component
    struct SphereColliderComponent
    {
        f32 Radius = 0.5f;
        glm::vec3 Offset = glm::vec3(0.0f);
        bool IsTrigger = false;
    };

    // Capsule collider component
    struct CapsuleColliderComponent
    {
        f32 Radius = 0.5f;
        f32 Height = 2.0f;
        glm::vec3 Offset = glm::vec3(0.0f);
        bool IsTrigger = false;
    };

    // Audio source component
    struct AudioSourceComponent
    {
        AssetHandle AudioClip = NullAssetHandle;
        f32 Volume = 1.0f;
        f32 Pitch = 1.0f;
        bool Loop = false;
        bool PlayOnAwake = false;
        bool Spatial = true;        // 3D audio
        f32 MinDistance = 1.0f;
        f32 MaxDistance = 100.0f;
    };
}
