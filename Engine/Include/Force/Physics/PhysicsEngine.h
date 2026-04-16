#pragma once

#include "Force/Core/Base.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <cstdint>

// Forward-declare Jolt types to keep the header clean
namespace JPH
{
    class PhysicsSystem;
    class BodyInterface;
    class TempAllocatorImpl;
    class JobSystemThreadPool;
    struct RayCastResult;
}

namespace Force
{
    struct PhysicsRayCastResult
    {
        glm::vec3 Point;
        glm::vec3 Normal;
        f32       Fraction  = 1.0f;  // t along ray [0,1]
        u32       BodyID    = UINT32_MAX;
        bool      Hit       = false;
    };

    struct PhysicsConfig
    {
        glm::vec3 Gravity            = { 0.0f, -9.81f, 0.0f };
        u32       MaxBodies          = 65536;
        u32       NumBodyMutexes     = 0;     // 0 = auto
        u32       MaxBodyPairs       = 65536;
        u32       MaxContactConstraints = 20480;
        u32       WorkerThreads      = 2;
        f32       FixedTimestep      = 1.0f / 60.0f;
        i32       CollisionSteps     = 1;
    };

    class PhysicsEngine
    {
    public:
        static void Init(const PhysicsConfig& config = {});
        static void Shutdown();

        // Step the simulation; call once per game tick
        static void Update(f32 dt);

        // Manual fixed-step (use if you manage your own accumulator)
        static void Step(f32 fixedDt, i32 collisionSteps = 1);

        static void SetGravity(const glm::vec3& g);
        static glm::vec3 GetGravity();

        // Ray cast — returns first hit
        static bool RayCast(const glm::vec3& origin, const glm::vec3& direction,
                            f32 maxDistance, PhysicsRayCastResult& outResult,
                            u32 layerMask = 0xFFFFFFFF);

        // Sphere cast
        static bool SphereCast(const glm::vec3& origin, f32 radius,
                               const glm::vec3& direction, f32 maxDistance,
                               PhysicsRayCastResult& outResult);

        static JPH::PhysicsSystem&      GetSystem();
        static JPH::BodyInterface&      GetBodyInterface();
        static JPH::TempAllocatorImpl*  GetTempAllocator() { return s_TempAllocator; }

        static bool IsInitialized() { return s_Initialized; }

    private:
        // Broad-phase layer setup (static/dynamic object layers)
        static void CreateLayers();

    private:
        static bool                          s_Initialized;
        static PhysicsConfig                 s_Config;
        static f32                           s_Accumulator;

        // Owned via raw pointers — Jolt objects not trivially movable
        static JPH::PhysicsSystem*           s_PhysicsSystem;
        static JPH::TempAllocatorImpl*       s_TempAllocator;
        static JPH::JobSystemThreadPool*     s_JobSystem;
    };
}
