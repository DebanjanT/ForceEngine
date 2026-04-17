#include "EditorLayer.h"
#include <Force/Core/Logger.h>
#include <imgui.h>
#include <imgui_internal.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtc/matrix_transform.hpp>

#ifdef FORCE_EDITOR_IMGUIZMO
#include <ImGuizmo.h>
#endif

namespace ForceEditor
{
    using namespace Force;

    void EditorLayer::ApplyStyle()
    {
        ImGui::StyleColorsDark();  // fill every slot with dark defaults first

        ImGuiStyle& s = ImGui::GetStyle();
        s.WindowRounding     = 2.f;
        s.ChildRounding      = 2.f;
        s.FrameRounding      = 3.f;
        s.GrabRounding       = 3.f;
        s.PopupRounding      = 2.f;
        s.ScrollbarRounding  = 3.f;
        s.TabRounding        = 3.f;
        s.WindowBorderSize   = 1.f;
        s.FrameBorderSize    = 0.f;
        s.IndentSpacing      = 14.f;
        s.ScrollbarSize      = 12.f;
        s.GrabMinSize        = 8.f;
        s.WindowPadding      = ImVec2(8.f, 6.f);
        s.FramePadding       = ImVec2(5.f, 3.f);
        s.ItemSpacing        = ImVec2(6.f, 4.f);

        const ImVec4 accent    = ImVec4(0.20f, 0.52f, 0.92f, 1.f);
        const ImVec4 accentDim = ImVec4(0.15f, 0.40f, 0.74f, 1.f);

        ImVec4* c = s.Colors;
        // ── Backgrounds ─────────────────────────  target: ~rgb(18,18,20)
        c[ImGuiCol_WindowBg]            = ImVec4(0.071f, 0.071f, 0.078f, 1.f);
        c[ImGuiCol_ChildBg]             = ImVec4(0.082f, 0.082f, 0.090f, 1.f);
        c[ImGuiCol_PopupBg]             = ImVec4(0.090f, 0.090f, 0.100f, 0.98f);
        c[ImGuiCol_FrameBg]             = ImVec4(0.118f, 0.118f, 0.133f, 1.f);
        c[ImGuiCol_FrameBgHovered]      = ImVec4(0.165f, 0.165f, 0.188f, 1.f);
        c[ImGuiCol_FrameBgActive]       = ImVec4(0.188f, 0.188f, 0.220f, 1.f);
        // ── Title / Menu ──────────────────────── target: ~rgb(10,10,12)
        c[ImGuiCol_TitleBg]             = ImVec4(0.039f, 0.039f, 0.047f, 1.f);
        c[ImGuiCol_TitleBgActive]       = ImVec4(0.059f, 0.059f, 0.071f, 1.f);
        c[ImGuiCol_TitleBgCollapsed]    = ImVec4(0.039f, 0.039f, 0.047f, 0.80f);
        c[ImGuiCol_MenuBarBg]           = ImVec4(0.039f, 0.039f, 0.047f, 1.f);
        // ── Borders ───────────────────────────────────────────────────────
        c[ImGuiCol_Border]              = ImVec4(0.188f, 0.188f, 0.220f, 1.f);
        c[ImGuiCol_BorderShadow]        = ImVec4(0.f,    0.f,    0.f,    0.f);
        c[ImGuiCol_Separator]           = ImVec4(0.157f, 0.157f, 0.188f, 1.f);
        c[ImGuiCol_SeparatorHovered]    = accentDim;
        c[ImGuiCol_SeparatorActive]     = accent;
        // ── Text ──────────────────────────────────────────────────────────
        c[ImGuiCol_Text]                = ImVec4(0.878f, 0.878f, 0.878f, 1.f);
        c[ImGuiCol_TextDisabled]        = ImVec4(0.380f, 0.380f, 0.420f, 1.f);
        // ── Buttons ───────────────────────────────────────────────────────
        c[ImGuiCol_Button]              = ImVec4(0.118f, 0.118f, 0.141f, 1.f);
        c[ImGuiCol_ButtonHovered]       = ImVec4(0.188f, 0.188f, 0.227f, 1.f);
        c[ImGuiCol_ButtonActive]        = accent;
        // ── Headers ───────────────────────────────────────────────────────
        c[ImGuiCol_Header]              = ImVec4(0.133f, 0.133f, 0.165f, 1.f);
        c[ImGuiCol_HeaderHovered]       = ImVec4(0.188f, 0.188f, 0.235f, 1.f);
        c[ImGuiCol_HeaderActive]        = accentDim;
        // ── Tabs ──────────────────────────────────────────────────────────
        c[ImGuiCol_Tab]                 = ImVec4(0.055f, 0.055f, 0.063f, 1.f);
        c[ImGuiCol_TabHovered]          = ImVec4(0.165f, 0.165f, 0.204f, 1.f);
        c[ImGuiCol_TabActive]           = ImVec4(0.118f, 0.118f, 0.149f, 1.f);
        c[ImGuiCol_TabUnfocused]        = ImVec4(0.039f, 0.039f, 0.047f, 1.f);
        c[ImGuiCol_TabUnfocusedActive]  = ImVec4(0.086f, 0.086f, 0.102f, 1.f);
        // ── Scrollbar ─────────────────────────────────────────────────────
        c[ImGuiCol_ScrollbarBg]         = ImVec4(0.031f, 0.031f, 0.039f, 1.f);
        c[ImGuiCol_ScrollbarGrab]       = ImVec4(0.196f, 0.196f, 0.235f, 1.f);
        c[ImGuiCol_ScrollbarGrabHovered]= ImVec4(0.267f, 0.267f, 0.318f, 1.f);
        c[ImGuiCol_ScrollbarGrabActive] = accent;
        // ── Misc ──────────────────────────────────────────────────────────
        c[ImGuiCol_CheckMark]           = accent;
        c[ImGuiCol_SliderGrab]          = accentDim;
        c[ImGuiCol_SliderGrabActive]    = accent;
        c[ImGuiCol_ResizeGrip]          = ImVec4(0.f, 0.f, 0.f, 0.f);
        c[ImGuiCol_ResizeGripHovered]   = accent;
        c[ImGuiCol_ResizeGripActive]    = accent;
        c[ImGuiCol_DockingPreview]      = ImVec4(accent.x, accent.y, accent.z, 0.50f);
        c[ImGuiCol_DockingEmptyBg]      = ImVec4(0.039f, 0.039f, 0.047f, 1.f);
        c[ImGuiCol_NavHighlight]        = accent;
        c[ImGuiCol_TableHeaderBg]       = ImVec4(0.075f, 0.075f, 0.090f, 1.f);
        c[ImGuiCol_TableBorderStrong]   = ImVec4(0.176f, 0.176f, 0.212f, 1.f);
        c[ImGuiCol_TableBorderLight]    = ImVec4(0.106f, 0.106f, 0.129f, 1.f);
    }

    EditorLayer::EditorLayer() {}
    EditorLayer::~EditorLayer() {}

    void EditorLayer::OnAttach()
    {
        ApplyStyle();
        NewScene();
        m_ContentBrowser.SetRootPath("Assets");
        m_ContentBrowser.OnAssetOpen = [this](const std::filesystem::path& p)
        {
            auto ext = p.extension().string();
            if (ext == ".fmat" || ext == ".mat")
                m_MaterialGraph.SetGraph(MaterialGraph::Create(), p.string());
        };

        FORCE_INFO("EditorLayer attached");
    }

    void EditorLayer::OnDetach()
    {
        FORCE_INFO("EditorLayer detached");
    }

    void EditorLayer::OnUpdate(f32 deltaTime)
    {
        m_Camera.OnUpdate(deltaTime, m_ViewportHovered, m_ViewportFocused);

        if (m_PlayState == EditorPlayState::Playing && m_ActiveScene)
            m_ActiveScene->OnUpdate(deltaTime);
    }

    void EditorLayer::OnRender()
    {
        // Resize
        auto& vp = m_ViewportSize;
        if (vp.x > 1 && vp.y > 1)
            m_Camera.OnResize((u32)vp.x, (u32)vp.y);
    }

    void EditorLayer::OnImGuiRender()
    {
#ifdef FORCE_EDITOR_IMGUIZMO
        ImGuizmo::BeginFrame();
#endif

        SetupDockspace();
        DrawMenuBar();
        DrawViewportPanel();

        m_Hierarchy.OnImGui();

        // Sync selection
        m_Properties.SetSelectedEntity(m_Hierarchy.GetSelectedEntity(), m_ActiveScene);
        m_Properties.OnImGui();

        m_ContentBrowser.OnImGui();
        m_OutputLog.OnImGui();
        m_MaterialGraph.OnImGui();

        if (m_ShowImGuiDemo) ImGui::ShowDemoWindow(&m_ShowImGuiDemo);
        HandleShortcuts();
        // ImGui::Render() + RenderDrawData handled by Renderer::EndFrame
    }

    // ─── Dockspace ────────────────────────────────────────────────────────────

    void EditorLayer::SetupDockspace()
    {
        ImGuiWindowFlags windowFlags =
            ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking |
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        const ImGuiViewport* vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(vp->WorkPos);
        ImGui::SetNextWindowSize(vp->WorkSize);
        ImGui::SetNextWindowViewport(vp->ID);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("##Editor", nullptr, windowFlags);
        ImGui::PopStyleVar(3);

        ImGuiID dockId = ImGui::GetID("EditorDock");
        ImGui::DockSpace(dockId, ImVec2(0, 0), ImGuiDockNodeFlags_PassthruCentralNode);

        if (m_FirstLayout)
        {
            m_FirstLayout = false;
            BuildDefaultLayout(dockId);
        }

        ImGui::End();
    }

    void EditorLayer::BuildDefaultLayout(ImGuiID dockId)
    {
        ImGui::DockBuilderRemoveNode(dockId);
        ImGui::DockBuilderAddNode(dockId, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockId, ImGui::GetMainViewport()->WorkSize);

        // Split: left panel | center+right
        ImGuiID dockLeft, dockMain;
        ImGui::DockBuilderSplitNode(dockId, ImGuiDir_Left, 0.20f, &dockLeft, &dockMain);

        // Split: center | right panel
        ImGuiID dockRight, dockCenter;
        ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Right, 0.25f, &dockRight, &dockCenter);

        // Split: center top (viewport) | bottom strip
        ImGuiID dockBottom, dockViewport;
        ImGui::DockBuilderSplitNode(dockCenter, ImGuiDir_Down, 0.28f, &dockBottom, &dockViewport);

        ImGui::DockBuilderDockWindow("Scene",           dockLeft);
        ImGui::DockBuilderDockWindow("Viewport",        dockViewport);
        ImGui::DockBuilderDockWindow("Details",         dockRight);
        ImGui::DockBuilderDockWindow("Content Browser", dockBottom);
        ImGui::DockBuilderDockWindow("Output Log",      dockBottom);
        ImGui::DockBuilderDockWindow("Material Graph",  dockViewport);

        ImGui::DockBuilderFinish(dockId);
    }

    // ─── Menu bar ─────────────────────────────────────────────────────────────

    void EditorLayer::DrawMenuBar()
    {
        if (!ImGui::BeginMainMenuBar()) return;

        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New Scene",        "Ctrl+N")) NewScene();
            if (ImGui::MenuItem("Open Scene...",    "Ctrl+O")) OpenScene();
            ImGui::Separator();
            if (ImGui::MenuItem("Save Scene",       "Ctrl+S")) SaveScene();
            if (ImGui::MenuItem("Save Scene As...", "Ctrl+Shift+S")) SaveSceneAs();
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "Alt+F4")) {}
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit"))
        {
            auto& undo = UndoRedoStack::Get();
            if (ImGui::MenuItem(("Undo: " + undo.GetUndoName()).c_str(), "Ctrl+Z", false, undo.CanUndo())) undo.Undo();
            if (ImGui::MenuItem(("Redo: " + undo.GetRedoName()).c_str(), "Ctrl+Y", false, undo.CanRedo())) undo.Redo();
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Window"))
        {
            ImGui::MenuItem("Scene",          nullptr, &m_Hierarchy.m_Open);
            ImGui::MenuItem("Details",        nullptr, &m_Properties.m_Open);
            ImGui::MenuItem("Content Browser",nullptr, &m_ContentBrowser.m_Open);
            ImGui::MenuItem("Output Log",     nullptr, &m_OutputLog.m_Open);
            ImGui::Separator();
            ImGui::MenuItem("Stats Overlay",  nullptr, &m_ShowStats);
            ImGui::MenuItem("ImGui Demo",     nullptr, &m_ShowImGuiDemo);
            ImGui::EndMenu();
        }

        // Play / Pause / Stop bar (centered)
        float bw = 90.f;
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - bw * 3.f) * 0.5f);

        bool isPlaying = m_PlayState == EditorPlayState::Playing;
        bool isPaused  = m_PlayState == EditorPlayState::Paused;
        bool isEdit    = m_PlayState == EditorPlayState::Edit;

        if (isPlaying) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f,0.6f,0.2f,1.f));
        if (ImGui::Button(isPlaying ? "  ||  Play" : "  > Play", ImVec2(bw, 0)))
            isEdit ? OnPlay() : (isPaused ? OnPlay() : OnPause());
        if (isPlaying) ImGui::PopStyleColor();

        ImGui::SameLine(0, 4);
        if (!isEdit) { ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f,0.2f,0.2f,1.f)); }
        if (ImGui::Button("  [] Stop", ImVec2(bw, 0)) && !isEdit) OnStop();
        if (!isEdit) ImGui::PopStyleColor();

        // FPS right-aligned
        float fps = ImGui::GetIO().Framerate;
        char buf[32]; snprintf(buf, sizeof(buf), " %.1f FPS ", fps);
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - ImGui::CalcTextSize(buf).x - 8.f);
        ImGui::TextDisabled("%s", buf);

        ImGui::EndMainMenuBar();
    }

    // ─── Viewport ─────────────────────────────────────────────────────────────

    void EditorLayer::DrawViewportPanel()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("Viewport");

        m_ViewportFocused = ImGui::IsWindowFocused();
        m_ViewportHovered = ImGui::IsWindowHovered();

        ImVec2 vSize = ImGui::GetContentRegionAvail();
        if (vSize.x < 1) vSize.x = 1;
        if (vSize.y < 1) vSize.y = 1;
        m_ViewportSize = vSize;
        m_ViewportPos  = ImGui::GetWindowPos();

        // Toolbar at top inside the viewport window
        DrawViewportToolbar();

        // Rendered image
        if (m_ViewportTex != 0)
            ImGui::Image(m_ViewportTex, vSize);

        // Gizmo overlay on selected entity
        auto sel = m_Hierarchy.GetSelectedEntity();
        if (sel != entt::null && m_ActiveScene)
        {
            auto& reg = m_ActiveScene->GetRegistry();
            if (reg.all_of<TransformComponent>(sel))
            {
                auto& tc = reg.get<TransformComponent>(sel);
                glm::mat4 xform = tc.GetTransform();
                if (m_Gizmo.DrawGizmo(xform, m_Camera.GetView(), m_Camera.GetProjection()))
                {
                    // Decompose back
                    tc.Position = glm::vec3(xform[3]);
                    tc.Scale    = glm::vec3(
                        glm::length(glm::vec3(xform[0])),
                        glm::length(glm::vec3(xform[1])),
                        glm::length(glm::vec3(xform[2])));
                    glm::mat3 rot(
                        glm::vec3(xform[0]) / tc.Scale.x,
                        glm::vec3(xform[1]) / tc.Scale.y,
                        glm::vec3(xform[2]) / tc.Scale.z);
                    tc.Rotation = glm::eulerAngles(glm::quat_cast(rot));
                }
                m_Gizmo.DrawViewCube(
                    const_cast<glm::mat4&>(m_Camera.GetView()),
                    m_Camera.GetProjection(), 80.f);
            }
        }

        if (m_ShowStats) DrawStatsOverlay();

        ImGui::End();
        ImGui::PopStyleVar();
    }

    void EditorLayer::DrawViewportToolbar()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 2));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f,0.3f,0.3f,0.5f));

        auto& cfg = m_Gizmo.GetConfig();

        // Gizmo mode buttons
        if (ImGui::Button("Q Sel"))  cfg.Operation = GizmoOperation::Translate;
        ImGui::SameLine();
        auto opColor = [&](GizmoOperation op) -> ImVec4
            { return cfg.Operation == op ? ImVec4(0.26f,0.59f,0.98f,1.f) : ImVec4(0,0,0,0); };

        ImGui::PushStyleColor(ImGuiCol_Button, opColor(GizmoOperation::Translate));
        if (ImGui::Button("W Move"))  cfg.Operation = GizmoOperation::Translate;
        ImGui::PopStyleColor();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, opColor(GizmoOperation::Rotate));
        if (ImGui::Button("E Rot"))   cfg.Operation = GizmoOperation::Rotate;
        ImGui::PopStyleColor();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, opColor(GizmoOperation::Scale));
        if (ImGui::Button("R Scale")) cfg.Operation = GizmoOperation::Scale;
        ImGui::PopStyleColor();
        ImGui::SameLine();

        ImGui::Separator();
        ImGui::SameLine();

        bool isWorld = cfg.Mode == GizmoMode::World;
        if (ImGui::Button(isWorld ? "World" : "Local"))
            cfg.Mode = isWorld ? GizmoMode::Local : GizmoMode::World;
        ImGui::SameLine();

        ImGui::Checkbox("Snap", &cfg.Snap);

        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(2);
    }

    void EditorLayer::DrawStatsOverlay()
    {
        ImVec2 winPos = m_ViewportPos;
        ImGui::SetNextWindowPos(ImVec2(winPos.x + 10, winPos.y + 50), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.5f);
        ImGui::SetNextWindowSize(ImVec2(180, 0));
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
                                 ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove |
                                 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking;
        ImGui::Begin("##stats", nullptr, flags);
        float fps = ImGui::GetIO().Framerate;
        ImGui::Text("FPS:      %.1f", fps);
        ImGui::Text("Frame:    %.2f ms", 1000.f / fps);
        ImGui::Text("Cam:      %.1f %.1f %.1f",
            m_Camera.GetPosition().x,
            m_Camera.GetPosition().y,
            m_Camera.GetPosition().z);
        ImGui::End();
    }

    // ─── Shortcuts ────────────────────────────────────────────────────────────

    void EditorLayer::HandleShortcuts()
    {
        if (!ImGui::IsAnyItemActive() && !ImGui::GetIO().WantTextInput)
        {
            auto& io = ImGui::GetIO();
            if (io.KeyCtrl)
            {
                if (ImGui::IsKeyPressed(ImGuiKey_N)) NewScene();
                if (ImGui::IsKeyPressed(ImGuiKey_S))
                    io.KeyShift ? SaveSceneAs() : SaveScene();
                if (ImGui::IsKeyPressed(ImGuiKey_Z)) UndoRedoStack::Get().Undo();
                if (ImGui::IsKeyPressed(ImGuiKey_Y)) UndoRedoStack::Get().Redo();
            }

            if (m_ViewportFocused)
            {
                auto& cfg = m_Gizmo.GetConfig();
                if (ImGui::IsKeyPressed(ImGuiKey_W)) cfg.Operation = GizmoOperation::Translate;
                if (ImGui::IsKeyPressed(ImGuiKey_E)) cfg.Operation = GizmoOperation::Rotate;
                if (ImGui::IsKeyPressed(ImGuiKey_R)) cfg.Operation = GizmoOperation::Scale;
            }

            if (ImGui::IsKeyPressed(ImGuiKey_F5)) OnPlay();
            if (ImGui::IsKeyPressed(ImGuiKey_F6)) OnStop();
        }
    }

    // ─── Scene ops ────────────────────────────────────────────────────────────

    void EditorLayer::NewScene()
    {
        m_EditorScene = CreateScope<Scene>();
        m_ActiveScene = m_EditorScene.get();
        m_Hierarchy.SetContext(m_ActiveScene);
        m_CurrentScenePath.clear();
        FORCE_INFO("New scene created");
    }

    void EditorLayer::OpenScene()
    {
        // TODO: file dialog
        FORCE_WARN("OpenScene: file dialog not yet implemented");
    }

    void EditorLayer::SaveScene()
    {
        if (m_CurrentScenePath.empty()) { SaveSceneAs(); return; }
        // SceneSerializer::Serialize(*m_EditorScene, m_CurrentScenePath);
        FORCE_INFO("Scene saved: {}", m_CurrentScenePath);
    }

    void EditorLayer::SaveSceneAs()
    {
        // TODO: file dialog
        FORCE_WARN("SaveSceneAs: file dialog not yet implemented");
    }

    // ─── Play state ───────────────────────────────────────────────────────────

    void EditorLayer::OnPlay()
    {
        if (m_PlayState == EditorPlayState::Edit)
        {
            // Run directly on editor scene (serialized snapshot TODO)
            m_ActiveScene = m_EditorScene.get();
            m_Hierarchy.SetContext(m_ActiveScene);
            FORCE_INFO("Play");
        }
        m_PlayState = EditorPlayState::Playing;
    }

    void EditorLayer::OnPause()
    {
        m_PlayState = EditorPlayState::Paused;
        FORCE_INFO("Paused");
    }

    void EditorLayer::OnStop()
    {
        m_PlayState   = EditorPlayState::Edit;
        m_ActiveScene = m_EditorScene.get();
        m_Hierarchy.SetContext(m_ActiveScene);
        FORCE_INFO("Stop");
    }
}
