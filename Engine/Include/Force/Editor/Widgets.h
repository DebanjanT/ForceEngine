#pragma once
#include "Force/Core/Base.h"
#include <imgui.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>

namespace Force::Widgets
{
    // -------------------------------------------------------------------------
    // Color gradient editor
    struct GradientStop
    {
        f32      Position = 0.0f; // [0,1]
        ImVec4   Color    = ImVec4(1, 1, 1, 1);
    };

    struct ColorGradient
    {
        std::vector<GradientStop> Stops;

        ColorGradient() { Stops.push_back({ 0.0f, ImVec4(0,0,0,1) }); Stops.push_back({ 1.0f, ImVec4(1,1,1,1) }); }
        ImVec4 Sample(f32 t) const;
        void   Sort();
    };

    // Returns true if gradient was modified
    bool GradientEditor(const char* label, ColorGradient& gradient,
                        float width = 0.0f, float height = 20.0f);

    // -------------------------------------------------------------------------
    // Curve editor (piecewise linear, up to 16 control points)
    struct CurvePoint { f32 X, Y; };

    struct Curve
    {
        std::vector<CurvePoint> Points;
        Curve() { Points.push_back({ 0.0f, 0.0f }); Points.push_back({ 1.0f, 1.0f }); }
        f32 Evaluate(f32 x) const;
        void Sort();
    };

    // Returns true if curve was modified
    bool CurveEditor(const char* label, Curve& curve,
                     const ImVec2& size = ImVec2(200, 150));

    // -------------------------------------------------------------------------
    // Vector editors (compact inline)
    bool Vec2Edit(const char* label, glm::vec2& v, float speed = 0.01f);
    bool Vec3Edit(const char* label, glm::vec3& v, float speed = 0.01f);
    bool Vec4Edit(const char* label, glm::vec4& v, float speed = 0.01f);

    // -------------------------------------------------------------------------
    // Texture preview thumbnail (pass VkDescriptorSet cast to ImTextureID)
    void TexturePreview(const char* label, ImTextureID texID, ImVec2 size = ImVec2(64, 64));

    // -------------------------------------------------------------------------
    // Searchable combo box
    bool SearchableCombo(const char* label, int& selectedIndex,
                         const std::vector<std::string>& items, float width = 0.0f);

    // -------------------------------------------------------------------------
    // Collapsible section header with icon
    bool SectionHeader(const char* label, bool defaultOpen = true);

    // -------------------------------------------------------------------------
    // Styled button variants
    bool PrimaryButton(const char* label, const ImVec2& size = ImVec2(0, 0));
    bool DangerButton(const char* label, const ImVec2& size = ImVec2(0, 0));
    bool IconButton(const char* icon, const char* tooltip = nullptr);
}
