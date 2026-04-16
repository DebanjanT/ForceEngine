#include "Force/Physics/PhysicsDebugRenderer.h"

#ifdef FORCE_PHYSICS_JOLT

#include "Force/Renderer/DebugRenderer.h"
#include <Jolt/Jolt.h>

JPH_SUPPRESS_WARNINGS

namespace Force
{
    PhysicsDebugRenderer& PhysicsDebugRenderer::Get()
    {
        static PhysicsDebugRenderer instance;
        return instance;
    }

    void PhysicsDebugRenderer::DrawLine(JPH::RVec3Arg from, JPH::RVec3Arg to,
                                         JPH::ColorArg color)
    {
        if (!m_Enabled) return;
        glm::vec4 c(color.r / 255.0f, color.g / 255.0f,
                    color.b / 255.0f, color.a / 255.0f);
        Force::DebugRenderer::DrawLine(
            { from.GetX(), from.GetY(), from.GetZ() },
            { to.GetX(),   to.GetY(),   to.GetZ()   }, c);
    }

    void PhysicsDebugRenderer::DrawTriangle(JPH::RVec3Arg v1, JPH::RVec3Arg v2,
                                             JPH::RVec3Arg v3, JPH::ColorArg color,
                                             ECastShadow /*castShadow*/)
    {
        if (!m_Enabled) return;
        glm::vec4 c(color.r / 255.0f, color.g / 255.0f,
                    color.b / 255.0f, color.a / 255.0f);
        Force::DebugRenderer::DrawLine({ v1.GetX(), v1.GetY(), v1.GetZ() },
                                       { v2.GetX(), v2.GetY(), v2.GetZ() }, c);
        Force::DebugRenderer::DrawLine({ v2.GetX(), v2.GetY(), v2.GetZ() },
                                       { v3.GetX(), v3.GetY(), v3.GetZ() }, c);
        Force::DebugRenderer::DrawLine({ v3.GetX(), v3.GetY(), v3.GetZ() },
                                       { v1.GetX(), v1.GetY(), v1.GetZ() }, c);
    }

    void PhysicsDebugRenderer::DrawText3D(JPH::RVec3Arg /*position*/,
                                           const std::string_view& /*text*/,
                                           JPH::ColorArg /*color*/, float /*height*/)
    {
        // Text overlay — would need ImGui integration; skip for now
    }

    void PhysicsDebugRenderer::Render()
    {
        // Called after PhysicsEngine::Update each frame if debug draw is on.
        // Jolt's BodyManager::Draw() will call our DrawLine/DrawTriangle above.
    }
}

#endif // FORCE_PHYSICS_JOLT
