#include "Force/Editor/Widgets.h"
#include <imgui_internal.h>
#include <algorithm>
#include <cmath>

namespace Force::Widgets
{
    // -------------------------------------------------------------------------
    // ColorGradient

    ImVec4 ColorGradient::Sample(f32 t) const
    {
        if (Stops.empty()) return ImVec4(0,0,0,1);
        if (Stops.size() == 1 || t <= Stops.front().Position) return Stops.front().Color;
        if (t >= Stops.back().Position)  return Stops.back().Color;
        for (u32 i = 1; i < Stops.size(); i++)
        {
            if (t <= Stops[i].Position)
            {
                f32 span = Stops[i].Position - Stops[i-1].Position;
                f32 alpha = span > 0.0f ? (t - Stops[i-1].Position) / span : 0.0f;
                const ImVec4& a = Stops[i-1].Color;
                const ImVec4& b = Stops[i].Color;
                return ImVec4(a.x+(b.x-a.x)*alpha, a.y+(b.y-a.y)*alpha, a.z+(b.z-a.z)*alpha, a.w+(b.w-a.w)*alpha);
            }
        }
        return Stops.back().Color;
    }

    void ColorGradient::Sort()
    {
        std::sort(Stops.begin(), Stops.end(), [](const GradientStop& a, const GradientStop& b){ return a.Position < b.Position; });
    }

    bool GradientEditor(const char* label, ColorGradient& gradient, float width, float height)
    {
        if (width <= 0.0f) width = ImGui::GetContentRegionAvail().x;
        bool modified = false;

        ImVec2 canvasPos = ImGui::GetCursorScreenPos();
        ImVec2 canvasSize(width, height);
        ImDrawList* dl = ImGui::GetWindowDrawList();

        // Draw gradient bar
        const int steps = 64;
        for (int i = 0; i < steps; i++)
        {
            f32 t0 = (f32)i / steps, t1 = (f32)(i+1) / steps;
            ImVec4 c0 = gradient.Sample(t0), c1 = gradient.Sample(t1);
            dl->AddRectFilledMultiColor(
                ImVec2(canvasPos.x + t0 * width, canvasPos.y),
                ImVec2(canvasPos.x + t1 * width, canvasPos.y + height),
                ImGui::ColorConvertFloat4ToU32(c0), ImGui::ColorConvertFloat4ToU32(c1),
                ImGui::ColorConvertFloat4ToU32(c1), ImGui::ColorConvertFloat4ToU32(c0));
        }
        dl->AddRect(canvasPos, ImVec2(canvasPos.x + width, canvasPos.y + height), IM_COL32(100,100,100,255));

        // Draw and interact with stops
        ImGui::InvisibleButton(label, canvasSize);
        bool barHovered = ImGui::IsItemHovered();

        // Add stop on double-click
        if (barHovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            f32 t = (ImGui::GetIO().MousePos.x - canvasPos.x) / width;
            gradient.Stops.push_back({ glm::clamp(t, 0.0f, 1.0f), gradient.Sample(t) });
            gradient.Sort();
            modified = true;
        }

        static int s_DragIdx = -1;
        for (int i = 0; i < (int)gradient.Stops.size(); i++)
        {
            auto& stop = gradient.Stops[i];
            ImVec2 handlePos(canvasPos.x + stop.Position * width, canvasPos.y + height);
            ImU32 c = ImGui::ColorConvertFloat4ToU32(stop.Color);
            dl->AddTriangleFilled(
                ImVec2(handlePos.x - 5, handlePos.y),
                ImVec2(handlePos.x + 5, handlePos.y),
                ImVec2(handlePos.x,     handlePos.y + 8), c);
            dl->AddTriangle(
                ImVec2(handlePos.x - 5, handlePos.y),
                ImVec2(handlePos.x + 5, handlePos.y),
                ImVec2(handlePos.x,     handlePos.y + 8), IM_COL32(200,200,200,255));

            // Drag
            ImGui::SetCursorScreenPos(ImVec2(handlePos.x - 6, handlePos.y - 2));
            ImGui::InvisibleButton(("stop" + std::to_string(i)).c_str(), ImVec2(12, 12));
            if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
            {
                stop.Position = glm::clamp((ImGui::GetIO().MousePos.x - canvasPos.x) / width, 0.0f, 1.0f);
                gradient.Sort();
                modified = true;
            }
            // Edit color on double-click
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                ImGui::OpenPopup(("StopColor" + std::to_string(i)).c_str());
            if (ImGui::BeginPopup(("StopColor" + std::to_string(i)).c_str()))
            {
                if (ImGui::ColorPicker4("##color", (float*)&stop.Color)) modified = true;
                ImGui::EndPopup();
            }
        }
        ImGui::Dummy(ImVec2(0, 10));
        return modified;
    }

    // -------------------------------------------------------------------------
    // Curve

    f32 Curve::Evaluate(f32 x) const
    {
        if (Points.size() < 2) return 0.0f;
        for (u32 i = 1; i < Points.size(); i++)
        {
            if (x <= Points[i].X)
            {
                f32 t = (x - Points[i-1].X) / (Points[i].X - Points[i-1].X);
                return Points[i-1].Y + (Points[i].Y - Points[i-1].Y) * t;
            }
        }
        return Points.back().Y;
    }

    void Curve::Sort()
    {
        std::sort(Points.begin(), Points.end(), [](const CurvePoint& a, const CurvePoint& b){ return a.X < b.X; });
    }

    bool CurveEditor(const char* label, Curve& curve, const ImVec2& size)
    {
        ImDrawList* dl  = ImGui::GetWindowDrawList();
        ImVec2 origin   = ImGui::GetCursorScreenPos();
        bool   modified = false;

        dl->AddRectFilled(origin, ImVec2(origin.x+size.x, origin.y+size.y), IM_COL32(35,35,38,255));
        dl->AddRect(origin, ImVec2(origin.x+size.x, origin.y+size.y), IM_COL32(80,80,80,255));

        // Draw curve segments
        auto toScreen = [&](f32 x, f32 y) -> ImVec2 {
            return ImVec2(origin.x + x * size.x, origin.y + (1.0f - y) * size.y);
        };
        for (int i = 1; i < (int)curve.Points.size(); i++)
        {
            ImVec2 p0 = toScreen(curve.Points[i-1].X, curve.Points[i-1].Y);
            ImVec2 p1 = toScreen(curve.Points[i].X,   curve.Points[i].Y);
            dl->AddLine(p0, p1, IM_COL32(100, 220, 80, 255), 2.0f);
        }

        // Draw control points
        ImGui::SetCursorScreenPos(origin);
        ImGui::InvisibleButton(label, size);
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            ImVec2 m = ImGui::GetIO().MousePos;
            f32 x = (m.x - origin.x) / size.x;
            f32 y = 1.0f - (m.y - origin.y) / size.y;
            curve.Points.push_back({ glm::clamp(x,0.0f,1.0f), glm::clamp(y,0.0f,1.0f) });
            curve.Sort();
            modified = true;
        }

        for (int i = 0; i < (int)curve.Points.size(); i++)
        {
            ImVec2 sp = toScreen(curve.Points[i].X, curve.Points[i].Y);
            dl->AddCircleFilled(sp, 5.0f, IM_COL32(220, 180, 50, 255));
            ImGui::SetCursorScreenPos(ImVec2(sp.x - 5, sp.y - 5));
            ImGui::InvisibleButton(("pt" + std::to_string(i)).c_str(), ImVec2(10, 10));
            if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
            {
                ImVec2 m = ImGui::GetIO().MousePos;
                curve.Points[i].X = glm::clamp((m.x - origin.x) / size.x, 0.0f, 1.0f);
                curve.Points[i].Y = glm::clamp(1.0f - (m.y - origin.y) / size.y, 0.0f, 1.0f);
                curve.Sort();
                modified = true;
            }
        }
        return modified;
    }

    // -------------------------------------------------------------------------

    bool Vec2Edit(const char* label, glm::vec2& v, float speed)
    {
        return ImGui::DragFloat2(label, &v.x, speed);
    }

    bool Vec3Edit(const char* label, glm::vec3& v, float speed)
    {
        return ImGui::DragFloat3(label, &v.x, speed);
    }

    bool Vec4Edit(const char* label, glm::vec4& v, float speed)
    {
        return ImGui::DragFloat4(label, &v.x, speed);
    }

    void TexturePreview(const char* label, ImTextureID texID, ImVec2 size)
    {
        if (!texID) { ImGui::TextDisabled("[no texture]"); return; }
        ImGui::Image(texID, size);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", label);
    }

    bool SearchableCombo(const char* label, int& selectedIndex, const std::vector<std::string>& items, float width)
    {
        static char buf[128] = "";
        bool changed = false;
        if (width > 0) ImGui::SetNextItemWidth(width);
        const char* preview = (selectedIndex >= 0 && selectedIndex < (int)items.size()) ? items[selectedIndex].c_str() : "---";
        if (ImGui::BeginCombo(label, preview))
        {
            ImGui::InputText("##search", buf, sizeof(buf));
            for (int i = 0; i < (int)items.size(); i++)
            {
                if (buf[0] && items[i].find(buf) == std::string::npos) continue;
                if (ImGui::Selectable(items[i].c_str(), i == selectedIndex))
                { selectedIndex = i; changed = true; }
            }
            ImGui::EndCombo();
        }
        return changed;
    }

    bool SectionHeader(const char* label, bool defaultOpen)
    {
        ImGui::PushStyleColor(ImGuiCol_Header,        ImVec4(0.22f, 0.22f, 0.26f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.28f, 0.28f, 0.35f, 1.0f));
        bool open = ImGui::CollapsingHeader(label, defaultOpen ? ImGuiTreeNodeFlags_DefaultOpen : 0);
        ImGui::PopStyleColor(2);
        return open;
    }

    bool PrimaryButton(const char* label, const ImVec2& size)
    {
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.18f, 0.45f, 0.85f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.55f, 0.95f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.14f, 0.38f, 0.75f, 1.0f));
        bool pressed = ImGui::Button(label, size);
        ImGui::PopStyleColor(3);
        return pressed;
    }

    bool DangerButton(const char* label, const ImVec2& size)
    {
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.75f, 0.18f, 0.18f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.90f, 0.25f, 0.25f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.60f, 0.12f, 0.12f, 1.0f));
        bool pressed = ImGui::Button(label, size);
        ImGui::PopStyleColor(3);
        return pressed;
    }

    bool IconButton(const char* icon, const char* tooltip)
    {
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f,0.3f,0.3f,0.5f));
        bool pressed = ImGui::Button(icon);
        ImGui::PopStyleColor(2);
        if (tooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", tooltip);
        return pressed;
    }
}
