#pragma once
#include "Force/Core/Base.h"
#include "Force/Editor/SceneHierarchyPanel.h"
#include "Force/Editor/MaterialGraphPanel.h"
#include "Force/Editor/ViewportGizmo.h"
#include "Force/Editor/UndoRedo.h"
#include <imgui.h>
#include <string>

namespace Force
{
    struct EditorStyle
    {
        ImVec4 Accent       = ImVec4(0.26f, 0.59f, 0.98f, 1.0f);
        ImVec4 BgDark       = ImVec4(0.11f, 0.11f, 0.13f, 1.0f);
        ImVec4 BgMid        = ImVec4(0.16f, 0.16f, 0.18f, 1.0f);
        ImVec4 BgLight      = ImVec4(0.22f, 0.22f, 0.25f, 1.0f);
        ImVec4 Text         = ImVec4(0.90f, 0.90f, 0.90f, 1.0f);
        ImVec4 TextDisabled = ImVec4(0.50f, 0.50f, 0.50f, 1.0f);
        float  Rounding     = 4.0f;
    };

    class EditorLayer
    {
    public:
        static EditorLayer& Get();

        void OnAttach();
        void OnDetach();

        // Called every frame before rendering
        void Begin();
        // Called every frame after rendering
        void End();

        // Draw all editor panels inside the dockspace
        void OnImGui();

        void ApplyStyle(const EditorStyle& style = {});

        SceneHierarchyPanel& GetSceneHierarchy()  { return m_SceneHierarchy; }
        MaterialGraphPanel&  GetMaterialGraph()   { return m_MaterialGraph; }
        ViewportGizmo&       GetViewportGizmo()   { return m_Gizmo; }

        // Open material graph panel with a specific asset
        void OpenMaterialGraph(const std::string& assetPath);

    private:
        EditorLayer() = default;
        void DrawDockspace();
        void DrawMenuBar();
        void DrawViewportPanel();
        void DrawStatsPanel();
        void DrawUndoHistoryPanel();

        SceneHierarchyPanel m_SceneHierarchy;
        MaterialGraphPanel  m_MaterialGraph;
        ViewportGizmo       m_Gizmo;

        bool m_ShowStats        = true;
        bool m_ShowUndoHistory  = false;
        bool m_ShowImGuiDemo    = false;

        float m_FrameTime  = 0.0f;
        float m_FPS        = 0.0f;
    };
}
