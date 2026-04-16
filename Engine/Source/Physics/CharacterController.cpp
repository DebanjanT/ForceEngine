#include "Force/Physics/CharacterController.h"
#include "Force/Physics/PhysicsEngine.h"
#include "Force/Core/Logger.h"

#ifdef FORCE_PHYSICS_JOLT

#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>

JPH_SUPPRESS_WARNINGS

namespace Force
{
    CharacterController::~CharacterController()
    {
        Destroy();
    }

    void CharacterController::Init(const CharacterControllerDesc& desc)
    {
        // Capsule standing upright: Jolt capsule is along Y axis
        JPH::Ref<JPH::Shape> capsule = new JPH::CapsuleShape(
            desc.CapsuleHeight * 0.5f - desc.CapsuleRadius, desc.CapsuleRadius);

        // Offset so feet are at origin
        JPH::Ref<JPH::Shape> standing = new JPH::RotatedTranslatedShape(
            JPH::Vec3(0, desc.CapsuleHeight * 0.5f, 0),
            JPH::Quat::sIdentity(), capsule);

        JPH::CharacterVirtualSettings settings;
        settings.mShape              = standing;
        settings.mMaxSlopeAngle      = glm::radians(desc.MaxSlopeAngleDeg);
        settings.mMaxStrength        = 100.0f;
        settings.mMass               = desc.Mass;
        settings.mPenetrationRecoverySpeed = 1.0f;
        settings.mPredictiveContactDistance = 0.1f;
        settings.mUp                 = JPH::Vec3::sAxisY();

        auto* character = new JPH::CharacterVirtual(
            &settings,
            JPH::RVec3(desc.InitialPosition.x,
                        desc.InitialPosition.y,
                        desc.InitialPosition.z),
            JPH::Quat::sIdentity(),
            &PhysicsEngine::GetSystem());

        m_Character = character;
    }

    void CharacterController::Destroy()
    {
        if (m_Character)
        {
            delete static_cast<JPH::CharacterVirtual*>(m_Character);
            m_Character = nullptr;
        }
    }

    void CharacterController::Update(f32 dt, const glm::vec3& desiredVelocity, bool /*jump*/)
    {
        if (!m_Character) return;
        auto* ch = static_cast<JPH::CharacterVirtual*>(m_Character);

        ch->SetLinearVelocity(JPH::Vec3(
            desiredVelocity.x, desiredVelocity.y, desiredVelocity.z));

        JPH::CharacterVirtual::ExtendedUpdateSettings updateSettings;
        JPH::Vec3 gravity = PhysicsEngine::GetSystem().GetGravity();
        ch->ExtendedUpdate(dt, gravity, updateSettings,
                            JPH::BroadPhaseLayerFilter{},
                            JPH::ObjectLayerFilter{},
                            JPH::BodyFilter{},
                            JPH::ShapeFilter{},
                            *PhysicsEngine::GetTempAllocator());
    }

    glm::vec3 CharacterController::GetPosition() const
    {
        if (!m_Character) return glm::vec3(0.0f);
        auto p = static_cast<JPH::CharacterVirtual*>(m_Character)->GetPosition();
        return { p.GetX(), p.GetY(), p.GetZ() };
    }

    void CharacterController::SetPosition(const glm::vec3& pos)
    {
        if (m_Character)
            static_cast<JPH::CharacterVirtual*>(m_Character)->SetPosition(
                JPH::RVec3(pos.x, pos.y, pos.z));
    }

    glm::vec3 CharacterController::GetLinearVelocity() const
    {
        if (!m_Character) return glm::vec3(0.0f);
        auto v = static_cast<JPH::CharacterVirtual*>(m_Character)->GetLinearVelocity();
        return { v.GetX(), v.GetY(), v.GetZ() };
    }

    GroundState CharacterController::GetGroundState() const
    {
        if (!m_Character) return GroundState::InAir;
        switch (static_cast<JPH::CharacterVirtual*>(m_Character)->GetGroundState())
        {
            case JPH::CharacterBase::EGroundState::OnGround:      return GroundState::OnGround;
            case JPH::CharacterBase::EGroundState::OnSteepGround: return GroundState::OnSteepGround;
            case JPH::CharacterBase::EGroundState::NotSupported:  return GroundState::NotSupported;
            default:                                              return GroundState::InAir;
        }
    }

    glm::vec3 CharacterController::GetGroundNormal() const
    {
        if (!m_Character) return { 0, 1, 0 };
        auto n = static_cast<JPH::CharacterVirtual*>(m_Character)->GetGroundNormal();
        return { n.GetX(), n.GetY(), n.GetZ() };
    }

    Ref<CharacterController> CharacterController::CreateCharacter(const CharacterControllerDesc& desc)
    {
        auto cc = CreateRef<CharacterController>();
        cc->Init(desc);
        return cc;
    }
}

#endif // FORCE_PHYSICS_JOLT
