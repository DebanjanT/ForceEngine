#include "Force/Editor/ContentBrowserPanel.h"
#include <imgui_internal.h>
#include <algorithm>

namespace fs = std::filesystem;

namespace Force
{
    ContentBrowserPanel::ContentBrowserPanel()
    {
        SetRootPath("Assets");
    }

    void ContentBrowserPanel::SetRootPath(const std::string& path)
    {
        m_RootPath    = path;
        m_CurrentPath = path;
        m_NeedsRefresh = true;
    }

    void ContentBrowserPanel::RefreshDirectory()
    {
        m_Entries.clear();
        if (!fs::exists(m_CurrentPath)) return;

        std::string filter(m_SearchBuf);

        for (auto& entry : fs::directory_iterator(m_CurrentPath))
        {
            ContentEntry ce;
            ce.Path        = entry.path();
            ce.IsDirectory = entry.is_directory();
            ce.Extension   = ce.Path.extension().string();
            std::transform(ce.Extension.begin(), ce.Extension.end(), ce.Extension.begin(), ::tolower);

            if (!filter.empty())
            {
                std::string name = ce.Path.filename().string();
                std::transform(name.begin(), name.end(), name.begin(), ::tolower);
                if (name.find(filter) == std::string::npos) continue;
            }
            m_Entries.push_back(ce);
        }

        // Sort: dirs first, then by name
        std::sort(m_Entries.begin(), m_Entries.end(), [](const ContentEntry& a, const ContentEntry& b)
        {
            if (a.IsDirectory != b.IsDirectory) return a.IsDirectory > b.IsDirectory;
            return a.Path.filename() < b.Path.filename();
        });

        m_NeedsRefresh = false;
    }

    void ContentBrowserPanel::OnImGui()
    {
        if (!m_Open) return;
        ImGui::Begin("Content Browser", &m_Open);

        // ── Left: directory tree ────────────────────────────────────────────
        float treeW = 180.f;
        ImGui::BeginChild("##tree", ImVec2(treeW, 0), true);
        DrawDirectoryTree(m_RootPath);
        ImGui::EndChild();

        ImGui::SameLine();

        // ── Right: asset tiles ──────────────────────────────────────────────
        ImGui::BeginChild("##tiles", ImVec2(0, 0), false);

        // Toolbar
        DrawBreadcrumbs();
        ImGui::SameLine();
        ImGui::SetNextItemWidth(160.f);
        if (ImGui::InputText("##search", m_SearchBuf, sizeof(m_SearchBuf)))
            m_NeedsRefresh = true;
        ImGui::SameLine();
        ImGui::SliderFloat("##zoom", &m_TileSize, 32.f, 128.f, "%.0f");
        ImGui::Separator();

        if (m_NeedsRefresh) RefreshDirectory();

        DrawAssetTiles();
        ImGui::EndChild();

        ImGui::End();
    }

    void ContentBrowserPanel::DrawDirectoryTree(const fs::path& dir)
    {
        if (!fs::exists(dir)) return;

        std::string name = dir == m_RootPath ? "Assets" : dir.filename().string();
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
        if (dir == m_CurrentPath) flags |= ImGuiTreeNodeFlags_Selected;

        // Check if has subdirs
        bool hasSubDirs = false;
        for (auto& e : fs::directory_iterator(dir))
            if (e.is_directory()) { hasSubDirs = true; break; }
        if (!hasSubDirs) flags |= ImGuiTreeNodeFlags_Leaf;

        bool open = ImGui::TreeNodeEx(name.c_str(), flags);
        if (ImGui::IsItemClicked())
        {
            m_CurrentPath  = dir;
            m_NeedsRefresh = true;
        }
        if (open)
        {
            for (auto& e : fs::directory_iterator(dir))
                if (e.is_directory()) DrawDirectoryTree(e.path());
            ImGui::TreePop();
        }
    }

    void ContentBrowserPanel::DrawBreadcrumbs()
    {
        // Build relative path components
        std::vector<fs::path> parts;
        fs::path p = m_CurrentPath;
        while (p != m_RootPath.parent_path() && p != p.parent_path())
        {
            parts.push_back(p);
            p = p.parent_path();
        }
        std::reverse(parts.begin(), parts.end());

        for (u32 i = 0; i < (u32)parts.size(); i++)
        {
            if (i > 0) { ImGui::SameLine(0, 2); ImGui::TextDisabled(">"); ImGui::SameLine(0, 2); }
            std::string seg = (parts[i] == m_RootPath) ? "Assets" : parts[i].filename().string();
            if (ImGui::SmallButton(seg.c_str()))
            {
                m_CurrentPath  = parts[i];
                m_NeedsRefresh = true;
            }
        }
    }

    static ImU32 ExtColor(const std::string& ext)
    {
        if (ext == ".png" || ext == ".jpg" || ext == ".tga" || ext == ".hdr") return IM_COL32(80,160,220,255);
        if (ext == ".obj" || ext == ".fbx" || ext == ".gltf" || ext == ".glb") return IM_COL32(220,160,60,255);
        if (ext == ".fmat" || ext == ".mat") return IM_COL32(160,220,80,255);
        if (ext == ".fscene" || ext == ".scene") return IM_COL32(200,100,200,255);
        if (ext == ".wav" || ext == ".mp3" || ext == ".ogg") return IM_COL32(100,200,200,255);
        if (ext == ".lua" || ext == ".cs")  return IM_COL32(240,200,60,255);
        return IM_COL32(160,160,160,255);
    }

    void ContentBrowserPanel::DrawAssetTiles()
    {
        float panelW = ImGui::GetContentRegionAvail().x;
        int columns = std::max(1, (int)(panelW / (m_TileSize + m_Padding)));

        ImGui::Columns(columns, nullptr, false);

        for (auto& entry : m_Entries)
        {
            std::string filename = entry.Path.filename().string();
            ImGui::PushID(filename.c_str());

            // Draw tile background
            ImVec2 tileSz{ m_TileSize, m_TileSize };
            ImDrawList* dl = ImGui::GetWindowDrawList();
            ImVec2 pos = ImGui::GetCursorScreenPos();
            ImU32 bgColor = entry.IsDirectory
                ? IM_COL32(60, 55, 40, 200)
                : IM_COL32(40, 40, 44, 200);

            // Invisible button for interaction
            bool dbl = false;
            ImGui::InvisibleButton("##tile", tileSz);
            if (ImGui::IsItemHovered()) bgColor = IM_COL32(70, 70, 80, 255);
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) dbl = true;

            dl->AddRectFilled(pos, ImVec2(pos.x + tileSz.x, pos.y + tileSz.y), bgColor, 6.f);
            dl->AddRect(pos, ImVec2(pos.x + tileSz.x, pos.y + tileSz.y),
                entry.IsDirectory ? IM_COL32(180,150,60,200) : ExtColor(entry.Extension), 6.f);

            // Icon letter
            ImVec2 center{ pos.x + tileSz.x * 0.5f, pos.y + tileSz.y * 0.45f };
            const char* icon = entry.IsDirectory ? "D" : "F";
            ImU32 iconCol = entry.IsDirectory ? IM_COL32(240,200,80,255) : ExtColor(entry.Extension);
            ImGui::GetWindowDrawList()->AddText(ImGui::GetFont(),
                m_TileSize * 0.35f,
                ImVec2(center.x - m_TileSize * 0.1f, center.y - m_TileSize * 0.18f),
                iconCol, icon);

            // Filename text below tile
            ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + tileSz.y + 2.f));
            ImGui::SetNextItemWidth(tileSz.x);
            ImGui::PushTextWrapPos(pos.x + tileSz.x);
            ImGui::TextUnformatted(filename.c_str());
            ImGui::PopTextWrapPos();

            if (dbl)
            {
                if (entry.IsDirectory) { m_CurrentPath = entry.Path; m_NeedsRefresh = true; }
                else if (OnAssetOpen) OnAssetOpen(entry.Path);
            }

            // Drag source
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
            {
                std::string pathStr = entry.Path.string();
                ImGui::SetDragDropPayload("CONTENT_ASSET", pathStr.c_str(), pathStr.size() + 1);
                ImGui::TextUnformatted(filename.c_str());
                ImGui::EndDragDropSource();
            }

            ImGui::PopID();
            ImGui::Spacing();
            ImGui::NextColumn();
        }
        ImGui::Columns(1);
    }
}
