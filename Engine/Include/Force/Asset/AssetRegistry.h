#pragma once

#include "Force/Core/Base.h"
#include "Force/Asset/Asset.h"
#include "Force/Asset/AssetTypes.h"
#include <filesystem>
#include <unordered_map>
#include <functional>
#include <mutex>

namespace Force
{
    // Callback for asset change notifications
    using AssetChangedCallback = std::function<void(AssetHandle, const std::string&)>;

    // Asset registry - manages all assets in the project
    class AssetRegistry
    {
    public:
        static AssetRegistry& Get();

        void Initialize(const std::filesystem::path& projectPath);
        void Shutdown();

        // Scan the project directory for assets
        void ScanDirectory(const std::filesystem::path& directory = "");
        void RefreshAsset(AssetHandle handle);

        // Asset metadata operations
        AssetHandle      ImportAsset(const std::filesystem::path& filepath);
        bool             IsAssetHandleValid(AssetHandle handle) const;
        AssetType        GetAssetType(AssetHandle handle) const;
        const AssetMetadata* GetMetadata(AssetHandle handle) const;
        AssetMetadata*   GetMutableMetadata(AssetHandle handle);

        // Path/handle lookups
        AssetHandle      GetAssetHandle(const std::filesystem::path& filepath) const;
        std::filesystem::path GetAssetPath(AssetHandle handle) const;
        std::filesystem::path GetAbsolutePath(AssetHandle handle) const;

        // Get all assets of a specific type
        std::vector<AssetHandle> GetAllAssetsOfType(AssetType type) const;

        // Load/unload assets
        template<typename T>
        Ref<T> GetAsset(AssetHandle handle);

        template<typename T>
        Ref<T> GetAsset(const std::filesystem::path& filepath);

        void UnloadAsset(AssetHandle handle);
        void UnloadAllAssets();

        // Hot-reload support
        void EnableFileWatching(bool enable);
        void ProcessFileChanges();
        void RegisterChangeCallback(AssetChangedCallback callback);

        // Project paths
        const std::filesystem::path& GetProjectPath() const { return m_ProjectPath; }
        const std::filesystem::path& GetAssetsPath() const { return m_AssetsPath; }

        // Metadata file management
        void SaveMetadata(AssetHandle handle);
        void SaveAllMetadata();

    private:
        AssetRegistry() = default;

        void ScanDirectoryRecursive(const std::filesystem::path& directory);
        AssetMetadata CreateMetadata(const std::filesystem::path& filepath);
        void LoadMetadataFile(const std::filesystem::path& metaPath);
        void SaveMetadataFile(const AssetMetadata& metadata);

        Ref<Asset> LoadAssetInternal(AssetHandle handle);

        std::filesystem::path GetMetaFilePath(const std::filesystem::path& assetPath) const;

    private:
        std::filesystem::path m_ProjectPath;
        std::filesystem::path m_AssetsPath;

        std::unordered_map<AssetHandle, AssetMetadata> m_AssetMetadata;
        std::unordered_map<std::string, AssetHandle>   m_PathToHandle; // relative path -> handle
        std::unordered_map<AssetHandle, Ref<Asset>>    m_LoadedAssets;

        std::vector<AssetChangedCallback> m_ChangeCallbacks;

        mutable std::mutex m_Mutex;
        bool m_FileWatchingEnabled = false;
        bool m_Initialized = false;
    };

    // Template implementations
    template<typename T>
    Ref<T> AssetRegistry::GetAsset(AssetHandle handle)
    {
        if (!IsAssetHandleValid(handle))
            return nullptr;

        std::lock_guard<std::mutex> lock(m_Mutex);

        // Check if already loaded
        auto it = m_LoadedAssets.find(handle);
        if (it != m_LoadedAssets.end())
        {
            return std::dynamic_pointer_cast<T>(it->second);
        }

        // Load the asset
        Ref<Asset> asset = LoadAssetInternal(handle);
        if (!asset)
            return nullptr;

        return std::dynamic_pointer_cast<T>(asset);
    }

    template<typename T>
    Ref<T> AssetRegistry::GetAsset(const std::filesystem::path& filepath)
    {
        AssetHandle handle = GetAssetHandle(filepath);
        if (handle == NullAssetHandle)
        {
            // Try to import it
            handle = ImportAsset(filepath);
        }
        return GetAsset<T>(handle);
    }
}
