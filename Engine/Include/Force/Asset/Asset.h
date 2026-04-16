#pragma once

#include "Force/Core/Base.h"
#include "Force/Asset/AssetTypes.h"

namespace Force
{
    // Base class for all loadable assets
    class Asset
    {
    public:
        virtual ~Asset() = default;

        virtual AssetType GetAssetType() const = 0;

        AssetHandle       GetHandle() const { return m_Handle; }
        const std::string& GetName() const { return m_Name; }
        const std::string& GetSourcePath() const { return m_SourcePath; }
        AssetLoadState    GetLoadState() const { return m_LoadState; }

        bool IsLoaded() const { return m_LoadState == AssetLoadState::Loaded; }

    protected:
        friend class AssetRegistry;
        friend class AssetImporter;

        AssetHandle    m_Handle;
        std::string    m_Name;
        std::string    m_SourcePath;
        AssetLoadState m_LoadState = AssetLoadState::Unloaded;
    };

    // Macro for defining asset types
    #define FORCE_ASSET_TYPE(Type) \
        static AssetType GetStaticType() { return AssetType::Type; } \
        virtual AssetType GetAssetType() const override { return GetStaticType(); }
}
