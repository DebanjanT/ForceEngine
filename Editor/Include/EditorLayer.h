#pragma once
#include <Force/Core/Base.h>
#include <Force/Core/ECS.h>
#include <Force/Editor/EditorCamera.h>
#include <Force/Editor/SceneHierarchyPanel.h>
#include <Force/Editor/PropertiesPanel.h>
#include <Force/Editor/ContentBrowserPanel.h>
#include <Force/Editor/OutputLogPanel.h>
#include <Force/Editor/MaterialGraphPanel.h>
#include <Force/Editor/ViewportGizmo.h>
#include <Force/Editor/UndoRedo.h>
#include <imgui.h>
#include <string>
#include <memory>

// Forward-declare engine EditorLayer so we can call Force::EditorLayer::Get()
namespace Force { class EditorLayer; }

namespace ForceEditor
{
    using namespace Force;

    enum class EditorPlayState { Edit, Playing, Paused };

    class EditorLayer
    {
    public:
        EditorLayer();
        ~EditorLayer();

        void OnAttach();
        void OnDetach();
        void OnUpdate(f32 deltaTime);
        void OnRender();
        void OnImGuiRender();

    private:
        void ApplyStyle();

        // ── Layout ────────────────────────────────────────────────────────────
        void SetupDockspace();
        void BuildDefaultLayout(ImGuiID dockId);

        // ── Panels ────────────────────────────────────────────────────────────
        void DrawMenuBar();
        void DrawViewportPanel();
        void DrawViewportToolbar();
        void DrawStatsOverlay();

        // ── Scene ops ─────────────────────────────────────────────────────────
        void NewScene();
        void OpenScene();
        void SaveScene();
        void SaveSceneAs();

        // ── Play state ────────────────────────────────────────────────────────
        void OnPlay();
        void OnPause();
        void OnStop();

        // ── Input ─────────────────────────────────────────────────────────────
        void HandleShortcuts();

        // ── Viewport helpers ──────────────────────────────────────────────────
        void DrawGizmoControls();

        // ── Members ───────────────────────────────────────────────────────────
        Scope<Scene>           m_EditorScene;
        Scene*                 m_ActiveScene = nullptr;

        EditorCamera           m_Camera;
        SceneHierarchyPanel    m_Hierarchy;
        PropertiesPanel        m_Properties;
        ContentBrowserPanel    m_ContentBrowser;
        OutputLogPanel         m_OutputLog;
        MaterialGraphPanel     m_MaterialGraph;
        ViewportGizmo          m_Gizmo;

        EditorPlayState        m_PlayState  = EditorPlayState::Edit;

        // Viewport
        ImVec2  m_ViewportSize  = { 1280, 720 };
        ImVec2  m_ViewportPos   = {};
        bool    m_ViewportFocused = false;
        bool    m_ViewportHovered = false;

        // ImTextureID of the renderer output (set by Renderer each frame)
        ImTextureID m_ViewportTex = 0;

        std::string m_CurrentScenePath;

        bool m_FirstLayout  = true;
        bool m_ShowStats    = true;
        bool m_ShowImGuiDemo = false;
    };
} // namespace ForceEditor
