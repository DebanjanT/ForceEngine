#pragma once
#include "Force/Core/Base.h"
#include <imgui.h>
#include <string>
#include <vector>
#include <filesystem>

namespace Force
{
    struct ContentEntry
    {
        std::filesystem::path Path;
        bool IsDirectory;
        std::string Extension; // lower-case
    };

    class ContentBrowserPanel
    {
    public:
        ContentBrowserPanel();
        void SetRootPath(const std::string& path);
        void OnImGui();

        bool m_Open = true;

        // Callback when an asset is double-clicked (path passed)
        std::function<void(const std::filesystem::path&)> OnAssetOpen;

    private:
        void RefreshDirectory();
        void DrawDirectoryTree(const std::filesystem::path& dir);
        void DrawAssetTiles();

        ImTextureID GetIconForExtension(const std::string& ext);
        void DrawBreadcrumbs();

        std::filesystem::path m_RootPath;
        std::filesystem::path m_CurrentPath;
        std::vector<ContentEntry> m_Entries;

        float m_TileSize    = 64.f;
        float m_Padding     = 12.f;
        char  m_SearchBuf[128] = {};

        bool  m_NeedsRefresh = true;
    };
}
