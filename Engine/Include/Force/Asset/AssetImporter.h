#pragma once

#include "Force/Core/Base.h"
#include "Force/Asset/Asset.h"
#include "Force/Asset/AssetTypes.h"
#include <filesystem>
#include <unordered_map>
#include <functional>

namespace Force
{
    // Import settings base
    struct ImportSettings
    {
        virtual ~ImportSettings() = default;
    };

    // Texture import settings
    struct TextureImportSettings : public ImportSettings
    {
        bool GenerateMipmaps = true;
        bool sRGB = true;
        bool FlipVertically = true;
        u32  MaxSize = 4096;
    };

    // Mesh import settings
    struct MeshImportSettings : public ImportSettings
    {
        bool ImportNormals = true;
        bool ImportTangents = true;
        bool ImportUVs = true;
        bool ImportMaterials = true;
        bool ImportAnimations = false;
        bool OptimizeMesh = true;
        f32  Scale = 1.0f;
        bool FlipUVs = false;
    };

    // Asset importer interface
    class IAssetImporter
    {
    public:
        virtual ~IAssetImporter() = default;
        virtual Ref<Asset> Import(const std::filesystem::path& path, const ImportSettings* settings) = 0;
        virtual bool CanImport(const std::string& extension) const = 0;
    };

    // Main asset importer - delegates to specialized importers
    class AssetImporter
    {
    public:
        static AssetImporter& Get();

        void Initialize();
        void Shutdown();

        // Register custom importer for specific extensions
        void RegisterImporter(Ref<IAssetImporter> importer);

        // Import asset from file
        Ref<Asset> Import(const std::filesystem::path& path, const ImportSettings* settings = nullptr);
        Ref<Asset> Import(AssetHandle handle, const AssetMetadata& metadata, const ImportSettings* settings = nullptr);

        // Check if file can be imported
        bool CanImport(const std::filesystem::path& path) const;

        // Get default settings for asset type
        template<typename T>
        static T GetDefaultSettings();

    private:
        AssetImporter() = default;

        IAssetImporter* FindImporter(const std::string& extension) const;

    private:
        std::vector<Ref<IAssetImporter>> m_Importers;
    };

    // Texture importer
    class TextureImporter : public IAssetImporter
    {
    public:
        Ref<Asset> Import(const std::filesystem::path& path, const ImportSettings* settings) override;
        bool CanImport(const std::string& extension) const override;
    };

    // Mesh importer (Assimp)
    class MeshImporter : public IAssetImporter
    {
    public:
        Ref<Asset> Import(const std::filesystem::path& path, const ImportSettings* settings) override;
        bool CanImport(const std::string& extension) const override;
    };
}
