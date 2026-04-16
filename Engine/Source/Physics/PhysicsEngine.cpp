#include "Force/Physics/PhysicsEngine.h"
#include "Force/Core/Logger.h"

#ifdef FORCE_PHYSICS_JOLT

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/PhysicsSettings.h>

JPH_SUPPRESS_WARNINGS

namespace Force
{
    // -------------------------------------------------------------------------
    // Broad-phase layer setup

    namespace Layers
    {
        static constexpr JPH::ObjectLayer STATIC  = 0;
        static constexpr JPH::ObjectLayer MOVING  = 1;
        static constexpr JPH::ObjectLayer SENSOR  = 2;
        static constexpr JPH::ObjectLayer NUM     = 3;
    }

    namespace BPLayers
    {
        static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
        static constexpr JPH::BroadPhaseLayer MOVING(1);
        static constexpr u32 NUM = 2;
    }

    class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface
    {
    public:
        BPLayerInterfaceImpl()
        {
            m_ObjectToBP[Layers::STATIC] = BPLayers::NON_MOVING;
            m_ObjectToBP[Layers::MOVING] = BPLayers::MOVING;
            m_ObjectToBP[Layers::SENSOR] = BPLayers::MOVING;
        }
        u32 GetNumBroadPhaseLayers() const override { return BPLayers::NUM; }
        JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer l) const override
        { return m_ObjectToBP[l]; }
#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
        const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer l) const override
        { return l == BPLayers::NON_MOVING ? "NON_MOVING" : "MOVING"; }
#endif
    private:
        JPH::BroadPhaseLayer m_ObjectToBP[Layers::NUM];
    };

    class ObjVsBPFilter final : public JPH::ObjectVsBroadPhaseLayerFilter
    {
    public:
        bool ShouldCollide(JPH::ObjectLayer l, JPH::BroadPhaseLayer b) const override
        {
            if (l == Layers::STATIC)  return b == BPLayers::MOVING;
            if (l == Layers::MOVING)  return true;
            if (l == Layers::SENSOR)  return b == BPLayers::MOVING;
            return false;
        }
    };

    class ObjVsObjFilter final : public JPH::ObjectLayerPairFilter
    {
    public:
        bool ShouldCollide(JPH::ObjectLayer a, JPH::ObjectLayer b) const override
        {
            if (a == Layers::STATIC) return b == Layers::MOVING;
            if (a == Layers::MOVING) return b != Layers::STATIC || b == Layers::MOVING;
            return false;
        }
    };

    static BPLayerInterfaceImpl  s_BPLayerInterface;
    static ObjVsBPFilter         s_ObjVsBP;
    static ObjVsObjFilter        s_ObjVsObj;

    // -------------------------------------------------------------------------
    // Statics
    bool                       PhysicsEngine::s_Initialized  = false;
    PhysicsConfig              PhysicsEngine::s_Config;
    f32                        PhysicsEngine::s_Accumulator  = 0.0f;
    JPH::PhysicsSystem*        PhysicsEngine::s_PhysicsSystem = nullptr;
    JPH::TempAllocatorImpl*    PhysicsEngine::s_TempAllocator = nullptr;
    JPH::JobSystemThreadPool*  PhysicsEngine::s_JobSystem     = nullptr;

    // -------------------------------------------------------------------------
    void PhysicsEngine::Init(const PhysicsConfig& config)
    {
        if (s_Initialized) return;
        s_Config = config;

        JPH::RegisterDefaultAllocator();
        JPH::Factory::sInstance = new JPH::Factory();
        JPH::RegisterTypes();

        s_TempAllocator = new JPH::TempAllocatorImpl(16 * 1024 * 1024); // 16 MB
        s_JobSystem     = new JPH::JobSystemThreadPool(
            JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers,
            static_cast<int>(config.WorkerThreads));

        s_PhysicsSystem = new JPH::PhysicsSystem();
        s_PhysicsSystem->Init(
            config.MaxBodies, config.NumBodyMutexes,
            config.MaxBodyPairs, config.MaxContactConstraints,
            s_BPLayerInterface, s_ObjVsBP, s_ObjVsObj);

        s_PhysicsSystem->SetGravity(
            JPH::Vec3(config.Gravity.x, config.Gravity.y, config.Gravity.z));

        s_Initialized = true;
        FORCE_CORE_INFO("PhysicsEngine (Jolt) initialized — max bodies: {}",
                        config.MaxBodies);
    }

    void PhysicsEngine::Shutdown()
    {
        if (!s_Initialized) return;

        delete s_PhysicsSystem;  s_PhysicsSystem = nullptr;
        delete s_JobSystem;      s_JobSystem     = nullptr;
        delete s_TempAllocator;  s_TempAllocator = nullptr;

        JPH::UnregisterTypes();
        delete JPH::Factory::sInstance;
        JPH::Factory::sInstance = nullptr;

        s_Initialized = false;
        FORCE_CORE_INFO("PhysicsEngine shutdown");
    }

    void PhysicsEngine::Update(f32 dt)
    {
        if (!s_Initialized) return;
        s_Accumulator += dt;
        while (s_Accumulator >= s_Config.FixedTimestep)
        {
            Step(s_Config.FixedTimestep, s_Config.CollisionSteps);
            s_Accumulator -= s_Config.FixedTimestep;
        }
    }

    void PhysicsEngine::Step(f32 fixedDt, i32 collisionSteps)
    {
        s_PhysicsSystem->Update(fixedDt, collisionSteps,
                                 s_TempAllocator, s_JobSystem);
    }

    void PhysicsEngine::SetGravity(const glm::vec3& g)
    {
        if (s_PhysicsSystem)
            s_PhysicsSystem->SetGravity(JPH::Vec3(g.x, g.y, g.z));
    }

    glm::vec3 PhysicsEngine::GetGravity()
    {
        if (!s_PhysicsSystem) return glm::vec3(0, -9.81f, 0);
        auto g = s_PhysicsSystem->GetGravity();
        return { g.GetX(), g.GetY(), g.GetZ() };
    }

    bool PhysicsEngine::RayCast(const glm::vec3& origin, const glm::vec3& direction,
                                  f32 maxDistance, PhysicsRayCastResult& outResult,
                                  u32 /*layerMask*/)
    {
        if (!s_PhysicsSystem) return false;

        JPH::RRayCast ray{
            JPH::RVec3(origin.x, origin.y, origin.z),
            JPH::Vec3(direction.x * maxDistance,
                      direction.y * maxDistance,
                      direction.z * maxDistance)
        };

        JPH::RayCastResult hit;
        bool found = s_PhysicsSystem->GetNarrowPhaseQuery()
                         .CastRay(ray, hit);

        if (found)
        {
            outResult.Hit      = true;
            outResult.Fraction = hit.mFraction;
            outResult.BodyID   = hit.mBodyID.GetIndexAndSequenceNumber();
            auto pt            = ray.GetPointOnRay(hit.mFraction);
            outResult.Point    = { pt.GetX(), pt.GetY(), pt.GetZ() };
        }
        return found;
    }

    bool PhysicsEngine::SphereCast(const glm::vec3& origin, f32 radius,
                                    const glm::vec3& direction, f32 maxDistance,
                                    PhysicsRayCastResult& outResult)
    {
        // TODO: implement using JPH ShapeCast
        (void)origin; (void)radius; (void)direction; (void)maxDistance;
        outResult.Hit = false;
        return false;
    }

    JPH::PhysicsSystem& PhysicsEngine::GetSystem()
    {
        FORCE_CORE_ASSERT(s_Initialized, "PhysicsEngine not initialized");
        return *s_PhysicsSystem;
    }

    JPH::BodyInterface& PhysicsEngine::GetBodyInterface()
    {
        return s_PhysicsSystem->GetBodyInterface();
    }
}

#endif // FORCE_PHYSICS_JOLT
