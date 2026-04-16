#pragma once

#include "Force/Core/Base.h"

// Only compiled when Jolt is available
#ifdef FORCE_PHYSICS_JOLT

#include <Jolt/Jolt.h>
#include <Jolt/Renderer/DebugRendererSimple.h>

JPH_SUPPRESS_WARNINGS

namespace Force
{
    // Bridges Jolt's DebugRendererSimple API to Force::DebugRenderer draw calls
    class PhysicsDebugRenderer final : public JPH::DebugRendererSimple
    {
    public:
        static PhysicsDebugRenderer& Get();

        void DrawLine(JPH::RVec3Arg from, JPH::RVec3Arg to,
                      JPH::ColorArg color) override;

        void DrawTriangle(JPH::RVec3Arg v1, JPH::RVec3Arg v2, JPH::RVec3Arg v3,
                          JPH::ColorArg color,
                          ECastShadow castShadow) override;

        void DrawText3D(JPH::RVec3Arg position, const std::string_view& text,
                        JPH::ColorArg color, float height) override;

        // Call this after registering with Jolt to draw all physics shapes this frame
        void Render();

        bool IsEnabled() const { return m_Enabled; }
        void SetEnabled(bool e) { m_Enabled = e; }

    private:
        PhysicsDebugRenderer() = default;
        bool m_Enabled = true;
    };
}

#endif // FORCE_PHYSICS_JOLT
