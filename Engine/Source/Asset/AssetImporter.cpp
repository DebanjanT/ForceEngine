#include "Force/Asset/AssetImporter.h"
#include "Force/Asset/AssetRegistry.h"
#include "Force/Asset/TextureAsset.h"
#include "Force/Asset/MeshAsset.h"
#include "Force/Renderer/Texture.h"
#include "Force/Renderer/Mesh.h"
#include "Force/Core/Logger.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace Force
{
    AssetImporter& AssetImporter::Get()
    {
        static AssetImporter instance;
        return instance;
    }

    void AssetImporter::Initialize()
    {
        // Register built-in importers
        RegisterImporter(CreateRef<TextureImporter>());
        RegisterImporter(CreateRef<MeshImporter>());

        FORCE_CORE_INFO("AssetImporter initialized");
    }

    void AssetImporter::Shutdown()
    {
        m_Importers.clear();
    }

    void AssetImporter::RegisterImporter(Ref<IAssetImporter> importer)
    {
        m_Importers.push_back(importer);
    }

    Ref<Asset> AssetImporter::Import(const std::filesystem::path& path, const ImportSettings* settings)
    {
        std::string ext = path.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        IAssetImporter* importer = FindImporter(ext);
        if (!importer)
        {
            FORCE_CORE_ERROR("AssetImporter: No importer found for extension: {}", ext);
            return nullptr;
        }

        return importer->Import(path, settings);
    }

    Ref<Asset> AssetImporter::Import(AssetHandle handle, const AssetMetadata& metadata, const ImportSettings* settings)
    {
        std::filesystem::path absolutePath = AssetRegistry::Get().GetAbsolutePath(handle);
        return Import(absolutePath, settings);
    }

    bool AssetImporter::CanImport(const std::filesystem::path& path) const
    {
        std::string ext = path.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        return FindImporter(ext) != nullptr;
    }

    IAssetImporter* AssetImporter::FindImporter(const std::string& extension) const
    {
        for (const auto& importer : m_Importers)
        {
            if (importer->CanImport(extension))
            {
                return importer.get();
            }
        }
        return nullptr;
    }

    // -------------------------------------------------------------------------
    // TextureImporter
    // -------------------------------------------------------------------------
    Ref<Asset> TextureImporter::Import(const std::filesystem::path& path, const ImportSettings* settings)
    {
        const TextureImportSettings* texSettings = nullptr;
        TextureImportSettings defaultSettings;

        if (settings)
        {
            texSettings = dynamic_cast<const TextureImportSettings*>(settings);
        }
        if (!texSettings)
        {
            texSettings = &defaultSettings;
        }

        // Load image data with stb_image
        stbi_set_flip_vertically_on_load(texSettings->FlipVertically ? 1 : 0);

        int width, height, channels;
        unsigned char* data = stbi_load(path.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);

        if (!data)
        {
            FORCE_CORE_ERROR("TextureImporter: Failed to load image: {} - {}",
                             path.string(), stbi_failure_reason());
            return nullptr;
        }

        // Determine format
        VkFormat format = texSettings->sRGB ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;

        // Create texture
        Ref<Texture2D> texture = Texture2D::CreateFromData(
            static_cast<u32>(width),
            static_cast<u32>(height),
            format,
            data,
            static_cast<u64>(width * height * 4)
        );

        stbi_image_free(data);

        if (!texture)
        {
            FORCE_CORE_ERROR("TextureImporter: Failed to create texture: {}", path.string());
            return nullptr;
        }

        // Create asset
        auto asset = CreateRef<TextureAsset>();
        asset->m_Texture = texture;
        asset->m_Name = path.stem().string();
        asset->m_SourcePath = path.string();
        asset->m_LoadState = AssetLoadState::Loaded;

        FORCE_CORE_INFO("TextureImporter: Loaded {}x{} texture: {}",
                        width, height, path.filename().string());

        return asset;
    }

    bool TextureImporter::CanImport(const std::string& extension) const
    {
        return extension == ".png" || extension == ".jpg" || extension == ".jpeg" ||
               extension == ".tga" || extension == ".bmp" || extension == ".hdr";
    }

    // -------------------------------------------------------------------------
    // MeshImporter (Assimp)
    // -------------------------------------------------------------------------
    Ref<Asset> MeshImporter::Import(const std::filesystem::path& path, const ImportSettings* settings)
    {
        const MeshImportSettings* meshSettings = nullptr;
        MeshImportSettings defaultSettings;

        if (settings)
        {
            meshSettings = dynamic_cast<const MeshImportSettings*>(settings);
        }
        if (!meshSettings)
        {
            meshSettings = &defaultSettings;
        }

        Assimp::Importer importer;

        unsigned int flags = aiProcess_Triangulate | aiProcess_GenBoundingBoxes;

        if (meshSettings->ImportNormals)
            flags |= aiProcess_GenSmoothNormals;
        if (meshSettings->ImportTangents)
            flags |= aiProcess_CalcTangentSpace;
        if (meshSettings->OptimizeMesh)
            flags |= aiProcess_OptimizeMeshes | aiProcess_OptimizeGraph;
        if (meshSettings->FlipUVs)
            flags |= aiProcess_FlipUVs;

        const aiScene* scene = importer.ReadFile(path.string(), flags);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            FORCE_CORE_ERROR("MeshImporter: Assimp error: {}", importer.GetErrorString());
            return nullptr;
        }

        // Collect all vertices and indices
        std::vector<Vertex> vertices;
        std::vector<u32> indices;
        std::vector<SubMesh> submeshes;
        std::vector<std::string> materialSlots;

        glm::vec3 boundsMin(FLT_MAX);
        glm::vec3 boundsMax(-FLT_MAX);

        // Process all meshes in the scene
        for (unsigned int m = 0; m < scene->mNumMeshes; ++m)
        {
            aiMesh* mesh = scene->mMeshes[m];

            SubMesh submesh;
            submesh.BaseVertex = static_cast<u32>(vertices.size());
            submesh.BaseIndex = static_cast<u32>(indices.size());

            // Process vertices
            for (unsigned int v = 0; v < mesh->mNumVertices; ++v)
            {
                Vertex vertex{};

                vertex.Position = glm::vec3(
                    mesh->mVertices[v].x,
                    mesh->mVertices[v].y,
                    mesh->mVertices[v].z
                ) * meshSettings->Scale;

                if (mesh->HasNormals())
                {
                    vertex.Normal = glm::vec3(
                        mesh->mNormals[v].x,
                        mesh->mNormals[v].y,
                        mesh->mNormals[v].z
                    );
                }

                if (mesh->mTextureCoords[0])
                {
                    vertex.TexCoord = glm::vec2(
                        mesh->mTextureCoords[0][v].x,
                        mesh->mTextureCoords[0][v].y
                    );
                }

                if (mesh->HasTangentsAndBitangents())
                {
                    vertex.Tangent = glm::vec3(
                        mesh->mTangents[v].x,
                        mesh->mTangents[v].y,
                        mesh->mTangents[v].z
                    );
                }

                vertices.push_back(vertex);

                // Update bounds
                boundsMin = glm::min(boundsMin, vertex.Position);
                boundsMax = glm::max(boundsMax, vertex.Position);
            }

            // Process indices
            for (unsigned int f = 0; f < mesh->mNumFaces; ++f)
            {
                aiFace& face = mesh->mFaces[f];
                for (unsigned int i = 0; i < face.mNumIndices; ++i)
                {
                    indices.push_back(face.mIndices[i]);
                }
            }

            submesh.IndexCount = static_cast<u32>(indices.size()) - submesh.BaseIndex;
            submesh.MaterialIndex = mesh->mMaterialIndex;

            // Get bounding box from Assimp
            submesh.BoundingBoxMin = glm::vec3(mesh->mAABB.mMin.x, mesh->mAABB.mMin.y, mesh->mAABB.mMin.z) * meshSettings->Scale;
            submesh.BoundingBoxMax = glm::vec3(mesh->mAABB.mMax.x, mesh->mAABB.mMax.y, mesh->mAABB.mMax.z) * meshSettings->Scale;

            submeshes.push_back(submesh);
        }

        // Collect material names
        if (meshSettings->ImportMaterials)
        {
            for (unsigned int m = 0; m < scene->mNumMaterials; ++m)
            {
                aiMaterial* material = scene->mMaterials[m];
                aiString name;
                material->Get(AI_MATKEY_NAME, name);
                materialSlots.push_back(name.C_Str());
            }
        }

        // Create mesh source
        MeshSource source;
        source.Vertices = std::move(vertices);
        source.Indices = std::move(indices);
        source.SubMeshes = std::move(submeshes);
        source.BoundingBoxMin = boundsMin;
        source.BoundingBoxMax = boundsMax;

        // Create mesh
        auto mesh = CreateRef<Mesh>(path.stem().string());
        mesh->Create(source);

        // Create asset
        auto asset = CreateRef<MeshAsset>();
        asset->AddLOD({ mesh, 1.0f });
        asset->m_BoundsMin = boundsMin;
        asset->m_BoundsMax = boundsMax;
        asset->m_MaterialSlots = std::move(materialSlots);
        asset->m_Name = path.stem().string();
        asset->m_SourcePath = path.string();
        asset->m_LoadState = AssetLoadState::Loaded;

        FORCE_CORE_INFO("MeshImporter: Loaded mesh with {} vertices, {} indices: {}",
                        source.Vertices.size(), source.Indices.size(), path.filename().string());

        return asset;
    }

    bool MeshImporter::CanImport(const std::string& extension) const
    {
        return extension == ".obj" || extension == ".fbx" || extension == ".gltf" ||
               extension == ".glb" || extension == ".dae";
    }
}
