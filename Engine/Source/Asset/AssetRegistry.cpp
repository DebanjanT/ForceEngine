#include "Force/Asset/AssetRegistry.h"
#include "Force/Asset/AssetImporter.h"
#include "Force/Core/Logger.h"
#include <nlohmann/json.hpp>
#include <fstream>

namespace Force
{
    AssetRegistry& AssetRegistry::Get()
    {
        static AssetRegistry instance;
        return instance;
    }

    void AssetRegistry::Initialize(const std::filesystem::path& projectPath)
    {
        m_ProjectPath = projectPath;
        m_AssetsPath = projectPath / "Assets";

        if (!std::filesystem::exists(m_AssetsPath))
        {
            std::filesystem::create_directories(m_AssetsPath);
        }

        AssetImporter::Get().Initialize();

        ScanDirectory();

        m_Initialized = true;
        FORCE_CORE_INFO("AssetRegistry initialized: {}", m_ProjectPath.string());
    }

    void AssetRegistry::Shutdown()
    {
        SaveAllMetadata();
        UnloadAllAssets();

        m_AssetMetadata.clear();
        m_PathToHandle.clear();
        m_LoadedAssets.clear();

        AssetImporter::Get().Shutdown();

        m_Initialized = false;
    }

    void AssetRegistry::ScanDirectory(const std::filesystem::path& directory)
    {
        std::filesystem::path scanPath = directory.empty() ? m_AssetsPath : m_AssetsPath / directory;

        if (!std::filesystem::exists(scanPath))
        {
            FORCE_CORE_WARN("AssetRegistry: Directory does not exist: {}", scanPath.string());
            return;
        }

        ScanDirectoryRecursive(scanPath);
        FORCE_CORE_INFO("AssetRegistry: Scanned {} assets", m_AssetMetadata.size());
    }

    void AssetRegistry::ScanDirectoryRecursive(const std::filesystem::path& directory)
    {
        for (const auto& entry : std::filesystem::directory_iterator(directory))
        {
            if (entry.is_directory())
            {
                ScanDirectoryRecursive(entry.path());
            }
            else if (entry.is_regular_file())
            {
                std::string ext = entry.path().extension().string();

                // Skip .meta files
                if (ext == ".meta")
                    continue;

                // Check if we can import this file type
                if (GetAssetTypeFromExtension(ext) == AssetType::None)
                    continue;

                std::filesystem::path metaPath = GetMetaFilePath(entry.path());

                if (std::filesystem::exists(metaPath))
                {
                    // Load existing metadata
                    LoadMetadataFile(metaPath);
                }
                else
                {
                    // Create new metadata
                    ImportAsset(entry.path());
                }
            }
        }
    }

    AssetHandle AssetRegistry::ImportAsset(const std::filesystem::path& filepath)
    {
        std::filesystem::path absolutePath = filepath;
        if (filepath.is_relative())
        {
            absolutePath = m_AssetsPath / filepath;
        }

        if (!std::filesystem::exists(absolutePath))
        {
            FORCE_CORE_ERROR("AssetRegistry: File does not exist: {}", absolutePath.string());
            return NullAssetHandle;
        }

        // Get relative path
        std::filesystem::path relativePath = std::filesystem::relative(absolutePath, m_AssetsPath);
        std::string relativeStr = relativePath.string();

        // Check if already imported
        auto it = m_PathToHandle.find(relativeStr);
        if (it != m_PathToHandle.end())
        {
            return it->second;
        }

        // Create metadata
        AssetMetadata metadata = CreateMetadata(absolutePath);
        if (!metadata.IsValid)
        {
            FORCE_CORE_ERROR("AssetRegistry: Failed to create metadata for: {}", absolutePath.string());
            return NullAssetHandle;
        }

        // Store metadata
        m_AssetMetadata[metadata.Handle] = metadata;
        m_PathToHandle[relativeStr] = metadata.Handle;

        // Save .meta file
        SaveMetadataFile(metadata);

        FORCE_CORE_INFO("AssetRegistry: Imported asset: {} [{}]",
                        metadata.Name, AssetTypeToString(metadata.Type));

        return metadata.Handle;
    }

    AssetMetadata AssetRegistry::CreateMetadata(const std::filesystem::path& filepath)
    {
        AssetMetadata metadata;

        std::string ext = filepath.extension().string();
        metadata.Type = GetAssetTypeFromExtension(ext);

        if (metadata.Type == AssetType::None)
        {
            return metadata;
        }

        metadata.Handle = UUID::Generate();
        metadata.SourcePath = std::filesystem::relative(filepath, m_AssetsPath).string();
        metadata.Name = filepath.stem().string();
        metadata.LastModified = static_cast<u64>(
            std::filesystem::last_write_time(filepath).time_since_epoch().count()
        );
        metadata.IsValid = true;

        return metadata;
    }

    void AssetRegistry::LoadMetadataFile(const std::filesystem::path& metaPath)
    {
        try
        {
            std::ifstream file(metaPath);
            if (!file.is_open())
                return;

            nlohmann::json j;
            file >> j;

            AssetMetadata metadata;
            metadata.Handle = UUID::FromString(j["uuid"].get<std::string>());
            metadata.Type = static_cast<AssetType>(j["type"].get<u8>());
            metadata.SourcePath = j["source"].get<std::string>();
            metadata.Name = j["name"].get<std::string>();
            metadata.LastModified = j.value("lastModified", 0ull);
            metadata.IsValid = true;

            m_AssetMetadata[metadata.Handle] = metadata;
            m_PathToHandle[metadata.SourcePath] = metadata.Handle;
        }
        catch (const std::exception& e)
        {
            FORCE_CORE_ERROR("AssetRegistry: Failed to load metadata: {} - {}",
                             metaPath.string(), e.what());
        }
    }

    void AssetRegistry::SaveMetadataFile(const AssetMetadata& metadata)
    {
        std::filesystem::path assetPath = m_AssetsPath / metadata.SourcePath;
        std::filesystem::path metaPath = GetMetaFilePath(assetPath);

        try
        {
            nlohmann::json j;
            j["uuid"] = metadata.Handle.ToString();
            j["type"] = static_cast<u8>(metadata.Type);
            j["source"] = metadata.SourcePath;
            j["name"] = metadata.Name;
            j["lastModified"] = metadata.LastModified;

            std::ofstream file(metaPath);
            file << j.dump(2);
        }
        catch (const std::exception& e)
        {
            FORCE_CORE_ERROR("AssetRegistry: Failed to save metadata: {} - {}",
                             metaPath.string(), e.what());
        }
    }

    std::filesystem::path AssetRegistry::GetMetaFilePath(const std::filesystem::path& assetPath) const
    {
        return assetPath.string() + ".meta";
    }

    bool AssetRegistry::IsAssetHandleValid(AssetHandle handle) const
    {
        return handle != NullAssetHandle && m_AssetMetadata.find(handle) != m_AssetMetadata.end();
    }

    AssetType AssetRegistry::GetAssetType(AssetHandle handle) const
    {
        auto it = m_AssetMetadata.find(handle);
        return it != m_AssetMetadata.end() ? it->second.Type : AssetType::None;
    }

    const AssetMetadata* AssetRegistry::GetMetadata(AssetHandle handle) const
    {
        auto it = m_AssetMetadata.find(handle);
        return it != m_AssetMetadata.end() ? &it->second : nullptr;
    }

    AssetMetadata* AssetRegistry::GetMutableMetadata(AssetHandle handle)
    {
        auto it = m_AssetMetadata.find(handle);
        return it != m_AssetMetadata.end() ? &it->second : nullptr;
    }

    AssetHandle AssetRegistry::GetAssetHandle(const std::filesystem::path& filepath) const
    {
        std::string relPath;
        if (filepath.is_absolute())
        {
            relPath = std::filesystem::relative(filepath, m_AssetsPath).string();
        }
        else
        {
            relPath = filepath.string();
        }

        auto it = m_PathToHandle.find(relPath);
        return it != m_PathToHandle.end() ? it->second : NullAssetHandle;
    }

    std::filesystem::path AssetRegistry::GetAssetPath(AssetHandle handle) const
    {
        auto it = m_AssetMetadata.find(handle);
        return it != m_AssetMetadata.end() ? it->second.SourcePath : "";
    }

    std::filesystem::path AssetRegistry::GetAbsolutePath(AssetHandle handle) const
    {
        auto it = m_AssetMetadata.find(handle);
        return it != m_AssetMetadata.end() ? m_AssetsPath / it->second.SourcePath : "";
    }

    std::vector<AssetHandle> AssetRegistry::GetAllAssetsOfType(AssetType type) const
    {
        std::vector<AssetHandle> handles;
        for (const auto& [handle, metadata] : m_AssetMetadata)
        {
            if (metadata.Type == type)
            {
                handles.push_back(handle);
            }
        }
        return handles;
    }

    Ref<Asset> AssetRegistry::LoadAssetInternal(AssetHandle handle)
    {
        auto metaIt = m_AssetMetadata.find(handle);
        if (metaIt == m_AssetMetadata.end())
        {
            FORCE_CORE_ERROR("AssetRegistry: Invalid handle");
            return nullptr;
        }

        const AssetMetadata& metadata = metaIt->second;
        std::filesystem::path absolutePath = m_AssetsPath / metadata.SourcePath;

        Ref<Asset> asset = AssetImporter::Get().Import(handle, metadata, nullptr);
        if (!asset)
        {
            FORCE_CORE_ERROR("AssetRegistry: Failed to load asset: {}", metadata.SourcePath);
            return nullptr;
        }

        asset->m_Handle = handle;
        asset->m_Name = metadata.Name;
        asset->m_SourcePath = metadata.SourcePath;
        asset->m_LoadState = AssetLoadState::Loaded;

        m_LoadedAssets[handle] = asset;

        FORCE_CORE_INFO("AssetRegistry: Loaded asset: {}", metadata.Name);
        return asset;
    }

    void AssetRegistry::UnloadAsset(AssetHandle handle)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_LoadedAssets.erase(handle);
    }

    void AssetRegistry::UnloadAllAssets()
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_LoadedAssets.clear();
    }

    void AssetRegistry::RefreshAsset(AssetHandle handle)
    {
        UnloadAsset(handle);
        // Asset will be reloaded on next GetAsset call
    }

    void AssetRegistry::SaveMetadata(AssetHandle handle)
    {
        auto it = m_AssetMetadata.find(handle);
        if (it != m_AssetMetadata.end())
        {
            SaveMetadataFile(it->second);
        }
    }

    void AssetRegistry::SaveAllMetadata()
    {
        for (const auto& [handle, metadata] : m_AssetMetadata)
        {
            SaveMetadataFile(metadata);
        }
    }

    void AssetRegistry::EnableFileWatching(bool enable)
    {
        m_FileWatchingEnabled = enable;
        // TODO: Implement file watching with efsw
    }

    void AssetRegistry::ProcessFileChanges()
    {
        // TODO: Process queued file changes from file watcher
    }

    void AssetRegistry::RegisterChangeCallback(AssetChangedCallback callback)
    {
        m_ChangeCallbacks.push_back(std::move(callback));
    }
}
