#pragma once

#include "Force/Core/Base.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <cstdint>

namespace Force
{
    // -------------------------------------------------------------------------
    // Collision layers
    enum class PhysicsLayer : u8
    {
        Static   = 0,
        Dynamic  = 1,
        Sensor   = 2,
        Count
    };

    // -------------------------------------------------------------------------
    // Shape types
    enum class PhysicsShapeType
    {
        Box, Sphere, Capsule, Cylinder,
        ConvexHull, TriangleMesh, Compound
    };

    struct PhysicsShapeDesc
    {
        PhysicsShapeType Type        = PhysicsShapeType::Box;
        glm::vec3        HalfExtents = { 0.5f, 0.5f, 0.5f };  // Box
        f32              Radius      = 0.5f;                    // Sphere / Capsule / Cylinder
        f32              Height      = 1.0f;                    // Capsule / Cylinder
        // Trimesh / convex hull — caller supplies vertices
        const f32*       Vertices    = nullptr;
        u32              VertexCount = 0;
        const u32*       Indices     = nullptr;
        u32              IndexCount  = 0;
    };

    // -------------------------------------------------------------------------
    // Rigid body
    enum class BodyMotionType { Static, Kinematic, Dynamic };

    struct RigidBodyDesc
    {
        PhysicsShapeDesc  Shape;
        BodyMotionType    MotionType  = BodyMotionType::Dynamic;
        PhysicsLayer      Layer       = PhysicsLayer::Dynamic;
        glm::vec3         Position    = glm::vec3(0.0f);
        glm::quat         Rotation    = glm::quat(1, 0, 0, 0);
        f32               Mass        = 1.0f;
        f32               Friction    = 0.5f;
        f32               Restitution = 0.0f;
        f32               LinearDamping  = 0.05f;
        f32               AngularDamping = 0.05f;
        bool              IsSensor    = false;
        bool              StartActive = true;
    };

    class RigidBody
    {
    public:
        RigidBody() = default;
        ~RigidBody();

        void Create(const RigidBodyDesc& desc);
        void Destroy();

        // Transform
        void        SetPosition(const glm::vec3& pos);
        void        SetRotation(const glm::quat& rot);
        void        SetTransform(const glm::vec3& pos, const glm::quat& rot);
        glm::vec3   GetPosition()  const;
        glm::quat   GetRotation()  const;
        glm::mat4   GetTransformMatrix() const;

        // Velocities
        void        SetLinearVelocity (const glm::vec3& v);
        void        SetAngularVelocity(const glm::vec3& v);
        glm::vec3   GetLinearVelocity()  const;
        glm::vec3   GetAngularVelocity() const;

        // Forces
        void AddForce  (const glm::vec3& force);
        void AddImpulse(const glm::vec3& impulse);
        void AddTorque (const glm::vec3& torque);

        // State
        void Activate();
        void Deactivate();
        bool IsActive() const;
        bool IsValid()  const { return m_BodyID != UINT32_MAX; }

        u32 GetBodyID() const { return m_BodyID; }

    private:
        u32 m_BodyID = UINT32_MAX;
    };

    // -------------------------------------------------------------------------
    // Compound shape builder
    class CompoundShapeBuilder
    {
    public:
        void AddShape(const PhysicsShapeDesc& shape,
                      const glm::vec3& localPos    = glm::vec3(0.0f),
                      const glm::quat& localRot    = glm::quat(1, 0, 0, 0));

        // Returns a desc that can be used in RigidBodyDesc
        PhysicsShapeDesc Build() const;

    private:
        struct Entry { PhysicsShapeDesc Desc; glm::vec3 Pos; glm::quat Rot; };
        std::vector<Entry> m_Entries;
    };
}
