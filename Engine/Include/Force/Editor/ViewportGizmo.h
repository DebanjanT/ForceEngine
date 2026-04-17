#pragma once
#include "Force/Core/Base.h"
#include <glm/glm.hpp>
#include <vector>

namespace Force
{
    enum class GizmoOperation { Translate, Rotate, Scale, Universal };
    enum class GizmoMode      { World, Local };

    struct GizmoConfig
    {
        GizmoOperation Operation = GizmoOperation::Translate;
        GizmoMode      Mode      = GizmoMode::World;
        bool           Snap      = false;
        glm::vec3      SnapValue = glm::vec3(0.25f); // per-axis snap unit
        bool           DrawGrid  = true;
        float          GridSize  = 1.0f;
    };

    class ViewportGizmo
    {
    public:
        ViewportGizmo() = default;

        void SetConfig(const GizmoConfig& cfg) { m_Config = cfg; }
        GizmoConfig& GetConfig() { return m_Config; }

        // Draw transform gizmo for a single matrix.
        // Must be called inside an ImGui window that covers the viewport.
        // Returns true if the transform was modified this frame.
        bool DrawGizmo(glm::mat4& transform,
                       const glm::mat4& view,
                       const glm::mat4& proj);

        // Multi-select: draw a gizmo at the centroid, returns delta transform.
        bool DrawMultiGizmo(std::vector<glm::mat4*>& transforms,
                            const glm::mat4& view,
                            const glm::mat4& proj);

        // Draw a view-orientation cube (like UE/Unity top-right corner)
        void DrawViewCube(glm::mat4& view, const glm::mat4& proj,
                          float size = 80.0f);

        bool IsUsing() const;
        bool IsOver()  const;

        // Object picking via depth buffer readback
        // screenPos: mouse position in window-local coords
        // Returns entity ID from object ID buffer, or 0 if nothing hit
        u32  PickObject(const glm::vec2& screenPos, const u32* objectIdBuffer,
                        u32 bufferWidth, u32 bufferHeight);

    private:
        GizmoConfig m_Config;
    };
}
