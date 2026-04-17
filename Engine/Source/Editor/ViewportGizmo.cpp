#include "Force/Editor/ViewportGizmo.h"
#include "Force/Core/Logger.h"
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

#ifdef FORCE_EDITOR_IMGUIZMO
#include <ImGuizmo.h>
#endif

namespace Force
{
    bool ViewportGizmo::DrawGizmo(glm::mat4& transform,
                                   const glm::mat4& view,
                                   const glm::mat4& proj)
    {
#ifdef FORCE_EDITOR_IMGUIZMO
        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist();

        ImVec2 winPos  = ImGui::GetWindowPos();
        ImVec2 winSize = ImGui::GetWindowSize();
        ImGuizmo::SetRect(winPos.x, winPos.y, winSize.x, winSize.y);

        ImGuizmo::OPERATION op = ImGuizmo::TRANSLATE;
        switch (m_Config.Operation) {
            case GizmoOperation::Translate:  op = ImGuizmo::TRANSLATE;  break;
            case GizmoOperation::Rotate:     op = ImGuizmo::ROTATE;     break;
            case GizmoOperation::Scale:      op = ImGuizmo::SCALE;      break;
            case GizmoOperation::Universal:  op = ImGuizmo::UNIVERSAL;  break;
        }
        ImGuizmo::MODE mode = (m_Config.Mode == GizmoMode::World) ? ImGuizmo::WORLD : ImGuizmo::LOCAL;

        float snapValues[3] = { m_Config.SnapValue.x, m_Config.SnapValue.y, m_Config.SnapValue.z };
        const float* snap   = m_Config.Snap ? snapValues : nullptr;

        return ImGuizmo::Manipulate(
            glm::value_ptr(view),
            glm::value_ptr(proj),
            op, mode,
            glm::value_ptr(transform),
            nullptr, snap);
#else
        (void)transform; (void)view; (void)proj;
        return false;
#endif
    }

    bool ViewportGizmo::DrawMultiGizmo(std::vector<glm::mat4*>& transforms,
                                        const glm::mat4& view,
                                        const glm::mat4& proj)
    {
        if (transforms.empty()) return false;

        // Compute centroid transform
        glm::vec3 centroid(0.0f);
        for (auto* t : transforms)
            centroid += glm::vec3((*t)[3]);
        centroid /= (float)transforms.size();

        glm::mat4 centroidMat = glm::translate(glm::mat4(1.0f), centroid);
        glm::mat4 prevMat     = centroidMat;

        bool changed = DrawGizmo(centroidMat, view, proj);
        if (changed)
        {
            glm::mat4 delta = centroidMat * glm::inverse(prevMat);
            for (auto* t : transforms)
                *t = delta * (*t);
        }
        return changed;
    }

    void ViewportGizmo::DrawViewCube(glm::mat4& view, const glm::mat4& proj, float size)
    {
#ifdef FORCE_EDITOR_IMGUIZMO
        ImVec2 winPos  = ImGui::GetWindowPos();
        ImVec2 winSize = ImGui::GetWindowSize();
        ImGuizmo::ViewManipulate(
            glm::value_ptr(view),
            8.0f,  // camera distance
            ImVec2(winPos.x + winSize.x - size - 8, winPos.y + 8),
            ImVec2(size, size),
            0x10101080);
#else
        (void)view; (void)proj; (void)size;
#endif
    }

    bool ViewportGizmo::IsUsing() const
    {
#ifdef FORCE_EDITOR_IMGUIZMO
        return ImGuizmo::IsUsing();
#else
        return false;
#endif
    }

    bool ViewportGizmo::IsOver() const
    {
#ifdef FORCE_EDITOR_IMGUIZMO
        return ImGuizmo::IsOver();
#else
        return false;
#endif
    }

    u32 ViewportGizmo::PickObject(const glm::vec2& screenPos, const u32* objectIdBuffer,
                                   u32 bufferWidth, u32 bufferHeight)
    {
        i32 px = (i32)screenPos.x;
        i32 py = (i32)screenPos.y;
        if (px < 0 || py < 0 || px >= (i32)bufferWidth || py >= (i32)bufferHeight)
            return 0;
        return objectIdBuffer[py * bufferWidth + px];
    }
}
