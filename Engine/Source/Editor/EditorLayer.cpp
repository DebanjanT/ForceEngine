#include "Force/Editor/EditorLayer.h"
#include "Force/Core/Logger.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_vulkan.h>
#include <backends/imgui_impl_glfw.h>

#ifdef FORCE_EDITOR_IMGUIZMO
#include <ImGuizmo.h>
#endif

namespace Force
{
    EditorLayer& EditorLayer::Get()
    {
        static EditorLayer instance;
        return instance;
    }

    void EditorLayer::OnAttach()
    {
        ApplyStyle();
        FORCE_CORE_INFO("EditorLayer: attached");
    }

    void EditorLayer::OnDetach()
    {
        FORCE_CORE_INFO("EditorLayer: detached");
    }

    void EditorLayer::ApplyStyle(const EditorStyle& style)
    {
        ImGuiStyle& s = ImGui::GetStyle();
        s.WindowRounding    = style.Rounding;
        s.ChildRounding     = style.Rounding;
        s.FrameRounding     = style.Rounding;
        s.GrabRounding      = style.Rounding;
        s.PopupRounding     = style.Rounding;
        s.ScrollbarRounding = style.Rounding;
        s.TabRounding       = style.Rounding;
        s.WindowBorderSize  = 1.0f;
        s.FrameBorderSize   = 0.0f;
        s.IndentSpacing     = 14.0f;

        ImVec4* c = s.Colors;
        c[ImGuiCol_WindowBg]          = style.BgDark;
        c[ImGuiCol_ChildBg]           = style.BgMid;
        c[ImGuiCol_PopupBg]           = style.BgMid;
        c[ImGuiCol_FrameBg]           = style.BgLight;
        c[ImGuiCol_FrameBgHovered]    = ImVec4(0.26f, 0.26f, 0.30f, 1.0f);
        c[ImGuiCol_FrameBgActive]     = ImVec4(0.30f, 0.30f, 0.36f, 1.0f);
        c[ImGuiCol_TitleBg]           = style.BgDark;
        c[ImGuiCol_TitleBgActive]     = ImVec4(0.14f, 0.14f, 0.17f, 1.0f);
        c[ImGuiCol_MenuBarBg]         = style.BgDark;
        c[ImGuiCol_Header]            = ImVec4(0.20f, 0.20f, 0.25f, 1.0f);
        c[ImGuiCol_HeaderHovered]     = ImVec4(0.26f, 0.26f, 0.32f, 1.0f);
        c[ImGuiCol_HeaderActive]      = ImVec4(0.22f, 0.35f, 0.60f, 1.0f);
        c[ImGuiCol_Button]            = style.BgLight;
        c[ImGuiCol_ButtonHovered]     = ImVec4(0.28f, 0.28f, 0.35f, 1.0f);
        c[ImGuiCol_ButtonActive]      = style.Accent;
        c[ImGuiCol_Tab]               = style.BgMid;
        c[ImGuiCol_TabHovered]        = ImVec4(0.26f, 0.26f, 0.32f, 1.0f);
        c[ImGuiCol_TabActive]         = style.BgLight;
        c[ImGuiCol_TabUnfocused]      = style.BgDark;
        c[ImGuiCol_TabUnfocusedActive]= style.BgMid;
        c[ImGuiCol_DockingPreview]    = ImVec4(style.Accent.x, style.Accent.y, style.Accent.z, 0.7f);
        c[ImGuiCol_CheckMark]         = style.Accent;
        c[ImGuiCol_SliderGrab]        = style.Accent;
        c[ImGuiCol_SliderGrabActive]  = ImVec4(style.Accent.x + 0.1f, style.Accent.y + 0.1f, style.Accent.z + 0.1f, 1.0f);
        c[ImGuiCol_Text]              = style.Text;
        c[ImGuiCol_TextDisabled]      = style.TextDisabled;
        c[ImGuiCol_Separator]         = ImVec4(0.28f, 0.28f, 0.30f, 1.0f);
        c[ImGuiCol_ResizeGrip]        = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
        c[ImGuiCol_ResizeGripHovered] = style.Accent;
        c[ImGuiCol_ScrollbarBg]       = style.BgDark;
        c[ImGuiCol_ScrollbarGrab]     = style.BgLight;
        c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.30f, 0.30f, 0.35f, 1.0f);
    }

    void EditorLayer::Begin()
    {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

#ifdef FORCE_EDITOR_IMGUIZMO
        ImGuizmo::BeginFrame();
#endif
    }

    void EditorLayer::End()
    {
        // ImGui::Render() + RenderDrawData are called by Renderer::EndFrame
    }

    void EditorLayer::OnImGui()
    {
        DrawDockspace();
        DrawMenuBar();
        DrawViewportPanel();
        DrawStatsPanel();

        m_SceneHierarchy.OnImGui();
        m_MaterialGraph.OnImGui();

        if (m_ShowUndoHistory) DrawUndoHistoryPanel();
        if (m_ShowImGuiDemo)   ImGui::ShowDemoWindow(&m_ShowImGuiDemo);
    }

    void EditorLayer::DrawDockspace()
    {
        ImGuiWindowFlags windowFlags =
            ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking |
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        ImGui::Begin("DockSpace", nullptr, windowFlags);
        ImGui::PopStyleVar(3);

        ImGuiID dockId = ImGui::GetID("MainDockSpace");
        ImGui::DockSpace(dockId, ImVec2(0, 0), ImGuiDockNodeFlags_PassthruCentralNode);
        ImGui::End();
    }

    void EditorLayer::DrawMenuBar()
    {
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("New Scene"))    {}
                if (ImGui::MenuItem("Open Scene"))   {}
                if (ImGui::MenuItem("Save Scene", "Ctrl+S")) {}
                ImGui::Separator();
                if (ImGui::MenuItem("Exit"))         {}
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
                ImGui::MenuItem("Scene Hierarchy",  nullptr, &m_SceneHierarchy.m_Open);
                ImGui::MenuItem("Stats",            nullptr, &m_ShowStats);
                ImGui::MenuItem("Undo History",     nullptr, &m_ShowUndoHistory);
                ImGui::Separator();
                ImGui::MenuItem("ImGui Demo",       nullptr, &m_ShowImGuiDemo);
                ImGui::EndMenu();
            }
            // FPS in menu bar right side
            float fps = ImGui::GetIO().Framerate;
            char fpsStr[32];
            snprintf(fpsStr, sizeof(fpsStr), " %.1f FPS (%.2f ms) ", fps, 1000.0f / fps);
            float w = ImGui::CalcTextSize(fpsStr).x;
            ImGui::SetCursorPosX(ImGui::GetWindowWidth() - w - 8);
            ImGui::TextDisabled("%s", fpsStr);

            ImGui::EndMainMenuBar();
        }
    }

    void EditorLayer::DrawViewportPanel()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("Viewport");
        ImVec2 viewportSize = ImGui::GetContentRegionAvail();
        // Renderer target image is bound here by the caller via ImGui::Image(...)
        // The gizmo is drawn on top inside the same window
        ImGui::End();
        ImGui::PopStyleVar();
    }

    void EditorLayer::DrawStatsPanel()
    {
        if (!m_ShowStats) return;
        ImGui::Begin("Stats", &m_ShowStats, ImGuiWindowFlags_NoCollapse);
        ImGui::Text("FPS:       %.1f",  ImGui::GetIO().Framerate);
        ImGui::Text("Frame ms:  %.2f",  1000.0f / ImGui::GetIO().Framerate);
        ImGui::Text("Draw calls: -- ");
        ImGui::Text("Triangles:  -- ");
        ImGui::End();
    }

    void EditorLayer::DrawUndoHistoryPanel()
    {
        ImGui::Begin("Undo History", &m_ShowUndoHistory);
        auto& stack = UndoRedoStack::Get();
        ImGui::Text("Actions: %u  |  Undo: %s  |  Redo: %s",
            stack.GetHistorySize(),
            stack.CanUndo() ? stack.GetUndoName().c_str() : "(none)",
            stack.CanRedo() ? stack.GetRedoName().c_str() : "(none)");
        ImGui::End();
    }

    void EditorLayer::OpenMaterialGraph(const std::string& assetPath)
    {
        auto graph = MaterialGraph::Create();
        // Try to load existing JSON
        std::ifstream f(assetPath);
        if (f.good())
        {
            std::string json((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
            graph->DeserializeJSON(json);
        }
        m_MaterialGraph.SetGraph(graph, assetPath);
    }
}
