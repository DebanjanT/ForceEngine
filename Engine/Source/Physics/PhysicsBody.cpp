#include "Force/Physics/PhysicsBody.h"
#include "Force/Physics/PhysicsEngine.h"
#include "Force/Core/Logger.h"

#ifdef FORCE_PHYSICS_JOLT

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>

JPH_SUPPRESS_WARNINGS

namespace Force
{
    // -------------------------------------------------------------------------
    static JPH::ObjectLayer ToJoltLayer(PhysicsLayer l)
    {
        switch (l)
        {
            case PhysicsLayer::Static:  return 0;
            case PhysicsLayer::Dynamic: return 1;
            case PhysicsLayer::Sensor:  return 2;
            default:                    return 1;
        }
    }

    static JPH::EMotionType ToJoltMotion(BodyMotionType m)
    {
        switch (m)
        {
            case BodyMotionType::Static:    return JPH::EMotionType::Static;
            case BodyMotionType::Kinematic: return JPH::EMotionType::Kinematic;
            case BodyMotionType::Dynamic:   return JPH::EMotionType::Dynamic;
            default:                        return JPH::EMotionType::Dynamic;
        }
    }

    static JPH::Ref<JPH::Shape> BuildShape(const PhysicsShapeDesc& desc)
    {
        switch (desc.Type)
        {
            case PhysicsShapeType::Box:
                return new JPH::BoxShape(
                    JPH::Vec3(desc.HalfExtents.x, desc.HalfExtents.y, desc.HalfExtents.z));

            case PhysicsShapeType::Sphere:
                return new JPH::SphereShape(desc.Radius);

            case PhysicsShapeType::Capsule:
                return new JPH::CapsuleShape(desc.Height * 0.5f, desc.Radius);

            case PhysicsShapeType::Cylinder:
                return new JPH::CylinderShape(desc.Height * 0.5f, desc.Radius);

            case PhysicsShapeType::TriangleMesh:
            {
                if (!desc.Vertices || !desc.Indices) return nullptr;
                JPH::TriangleList tris;
                for (u32 i = 0; i + 2 < desc.IndexCount; i += 3)
                {
                    u32 i0 = desc.Indices[i], i1 = desc.Indices[i+1], i2 = desc.Indices[i+2];
                    tris.push_back(JPH::Triangle(
                        JPH::Float3(desc.Vertices[i0*3+0], desc.Vertices[i0*3+1], desc.Vertices[i0*3+2]),
                        JPH::Float3(desc.Vertices[i1*3+0], desc.Vertices[i1*3+1], desc.Vertices[i1*3+2]),
                        JPH::Float3(desc.Vertices[i2*3+0], desc.Vertices[i2*3+1], desc.Vertices[i2*3+2])));
                }
                JPH::MeshShapeSettings settings(tris);
                auto result = settings.Create();
                return result.IsValid() ? result.Get() : nullptr;
            }

            default:
                FORCE_CORE_WARN("PhysicsBody: unsupported shape type, defaulting to box");
                return new JPH::BoxShape(JPH::Vec3(0.5f, 0.5f, 0.5f));
        }
    }

    // -------------------------------------------------------------------------
    RigidBody::~RigidBody()
    {
        Destroy();
    }

    void RigidBody::Create(const RigidBodyDesc& desc)
    {
        auto shape = BuildShape(desc.Shape);
        if (!shape)
        {
            FORCE_CORE_ERROR("RigidBody: failed to build shape");
            return;
        }

        JPH::BodyCreationSettings settings(
            shape,
            JPH::RVec3(desc.Position.x, desc.Position.y, desc.Position.z),
            JPH::Quat(desc.Rotation.x, desc.Rotation.y, desc.Rotation.z, desc.Rotation.w),
            ToJoltMotion(desc.MotionType),
            ToJoltLayer(desc.Layer));

        if (desc.MotionType == BodyMotionType::Dynamic)
        {
            settings.mMassPropertiesOverride.mMass = desc.Mass;
            settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
        }
        settings.mFriction          = desc.Friction;
        settings.mRestitution       = desc.Restitution;
        settings.mLinearDamping     = desc.LinearDamping;
        settings.mAngularDamping    = desc.AngularDamping;
        settings.mIsSensor          = desc.IsSensor;

        auto& bi = PhysicsEngine::GetBodyInterface();
        JPH::Body* body = bi.CreateBody(settings);
        if (!body)
        {
            FORCE_CORE_ERROR("RigidBody: body creation failed (too many bodies?)");
            return;
        }

        m_BodyID = body->GetID().GetIndexAndSequenceNumber();

        if (desc.StartActive)
            bi.AddBody(body->GetID(), JPH::EActivation::Activate);
        else
            bi.AddBody(body->GetID(), JPH::EActivation::DontActivate);
    }

    void RigidBody::Destroy()
    {
        if (m_BodyID == UINT32_MAX) return;
        auto& bi = PhysicsEngine::GetBodyInterface();
        JPH::BodyID id = JPH::BodyID(m_BodyID);
        bi.RemoveBody(id);
        bi.DestroyBody(id);
        m_BodyID = UINT32_MAX;
    }

    static JPH::BodyID ToJoltID(u32 id) { return JPH::BodyID(id); }

    void RigidBody::SetPosition(const glm::vec3& pos)
    {
        PhysicsEngine::GetBodyInterface().SetPosition(ToJoltID(m_BodyID),
            JPH::RVec3(pos.x, pos.y, pos.z), JPH::EActivation::Activate);
    }

    void RigidBody::SetRotation(const glm::quat& rot)
    {
        PhysicsEngine::GetBodyInterface().SetRotation(ToJoltID(m_BodyID),
            JPH::Quat(rot.x, rot.y, rot.z, rot.w), JPH::EActivation::Activate);
    }

    void RigidBody::SetTransform(const glm::vec3& pos, const glm::quat& rot)
    {
        PhysicsEngine::GetBodyInterface().SetPositionAndRotation(ToJoltID(m_BodyID),
            JPH::RVec3(pos.x, pos.y, pos.z),
            JPH::Quat(rot.x, rot.y, rot.z, rot.w),
            JPH::EActivation::Activate);
    }

    glm::vec3 RigidBody::GetPosition() const
    {
        auto p = PhysicsEngine::GetBodyInterface().GetPosition(ToJoltID(m_BodyID));
        return { p.GetX(), p.GetY(), p.GetZ() };
    }

    glm::quat RigidBody::GetRotation() const
    {
        auto r = PhysicsEngine::GetBodyInterface().GetRotation(ToJoltID(m_BodyID));
        return glm::quat(r.GetW(), r.GetX(), r.GetY(), r.GetZ());
    }

    glm::mat4 RigidBody::GetTransformMatrix() const
    {
        glm::mat4 m = glm::mat4_cast(GetRotation());
        m[3] = glm::vec4(GetPosition(), 1.0f);
        return m;
    }

    void RigidBody::SetLinearVelocity(const glm::vec3& v)
    {
        PhysicsEngine::GetBodyInterface().SetLinearVelocity(ToJoltID(m_BodyID),
            JPH::Vec3(v.x, v.y, v.z));
    }

    void RigidBody::SetAngularVelocity(const glm::vec3& v)
    {
        PhysicsEngine::GetBodyInterface().SetAngularVelocity(ToJoltID(m_BodyID),
            JPH::Vec3(v.x, v.y, v.z));
    }

    glm::vec3 RigidBody::GetLinearVelocity() const
    {
        auto v = PhysicsEngine::GetBodyInterface().GetLinearVelocity(ToJoltID(m_BodyID));
        return { v.GetX(), v.GetY(), v.GetZ() };
    }

    glm::vec3 RigidBody::GetAngularVelocity() const
    {
        auto v = PhysicsEngine::GetBodyInterface().GetAngularVelocity(ToJoltID(m_BodyID));
        return { v.GetX(), v.GetY(), v.GetZ() };
    }

    void RigidBody::AddForce(const glm::vec3& f)
    {
        PhysicsEngine::GetBodyInterface().AddForce(ToJoltID(m_BodyID), JPH::Vec3(f.x,f.y,f.z));
    }

    void RigidBody::AddImpulse(const glm::vec3& i)
    {
        PhysicsEngine::GetBodyInterface().AddImpulse(ToJoltID(m_BodyID), JPH::Vec3(i.x,i.y,i.z));
    }

    void RigidBody::AddTorque(const glm::vec3& t)
    {
        PhysicsEngine::GetBodyInterface().AddTorque(ToJoltID(m_BodyID), JPH::Vec3(t.x,t.y,t.z));
    }

    void RigidBody::Activate()   { PhysicsEngine::GetBodyInterface().ActivateBody(ToJoltID(m_BodyID)); }
    void RigidBody::Deactivate() { PhysicsEngine::GetBodyInterface().DeactivateBody(ToJoltID(m_BodyID)); }
    bool RigidBody::IsActive() const
    { return PhysicsEngine::GetBodyInterface().IsActive(ToJoltID(m_BodyID)); }
}

#endif // FORCE_PHYSICS_JOLT
