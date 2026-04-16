#pragma once

#include "Force/Core/Base.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Force
{
    struct CharacterControllerDesc
    {
        f32       CapsuleRadius       = 0.3f;
        f32       CapsuleHeight       = 1.8f;   // standing full height
        f32       StepHeight          = 0.35f;
        f32       MaxSlopeAngleDeg    = 45.0f;
        f32       Mass                = 70.0f;
        glm::vec3 InitialPosition     = glm::vec3(0.0f);
    };

    enum class GroundState { OnGround, OnSteepGround, NotSupported, InAir };

    class CharacterController
    {
    public:
        CharacterController() = default;
        ~CharacterController();

        void Init(const CharacterControllerDesc& desc);
        void Destroy();

        // Called every physics step — sets desired velocity for this step
        void Update(f32 dt, const glm::vec3& desiredVelocity, bool jump = false);

        glm::vec3    GetPosition()     const;
        void         SetPosition(const glm::vec3& pos);
        glm::vec3    GetLinearVelocity() const;

        GroundState  GetGroundState()  const;
        bool         IsGrounded()      const { return GetGroundState() == GroundState::OnGround; }

        glm::vec3    GetGroundNormal() const;

        static Ref<CharacterController> CreateCharacter(const CharacterControllerDesc& desc);

    private:
        // Opaque pointer to JPH::CharacterVirtual — avoids Jolt headers in this header
        void* m_Character = nullptr;
    };
}
