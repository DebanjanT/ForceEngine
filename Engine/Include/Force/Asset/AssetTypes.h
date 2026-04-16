#pragma once

#include "Force/Core/Types.h"
#include <string>

namespace Force
{
    // Asset type enumeration
    enum class AssetType : u8
    {
        None = 0,
        Texture2D,
        TextureCube,
        Mesh,
        SkeletalMesh,
        Animation,
        Material,
        Shader,
        Audio,
        Scene,
        Prefab,
        Font,
        Script,
        PhysicsMaterial,
        Count
    };

    // Convert asset type to string
    inline const char* AssetTypeToString(AssetType type)
    {
        switch (type)
        {
            case AssetType::None:            return "None";
            case AssetType::Texture2D:       return "Texture2D";
            case AssetType::TextureCube:     return "TextureCube";
            case AssetType::Mesh:            return "Mesh";
            case AssetType::SkeletalMesh:    return "SkeletalMesh";
            case AssetType::Animation:       return "Animation";
            case AssetType::Material:        return "Material";
            case AssetType::Shader:          return "Shader";
            case AssetType::Audio:           return "Audio";
            case AssetType::Scene:           return "Scene";
            case AssetType::Prefab:          return "Prefab";
            case AssetType::Font:            return "Font";
            case AssetType::Script:          return "Script";
            case AssetType::PhysicsMaterial: return "PhysicsMaterial";
            default:                         return "Unknown";
        }
    }

    // Asset file extensions mapping
    inline AssetType GetAssetTypeFromExtension(const std::string& ext)
    {
        if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga" || ext == ".bmp" || ext == ".hdr")
            return AssetType::Texture2D;
        if (ext == ".obj" || ext == ".fbx" || ext == ".gltf" || ext == ".glb" || ext == ".dae")
            return AssetType::Mesh;
        if (ext == ".mat" || ext == ".material")
            return AssetType::Material;
        if (ext == ".glsl" || ext == ".vert" || ext == ".frag" || ext == ".comp" || ext == ".spv")
            return AssetType::Shader;
        if (ext == ".wav" || ext == ".mp3" || ext == ".ogg" || ext == ".flac")
            return AssetType::Audio;
        if (ext == ".scene" || ext == ".fscene")
            return AssetType::Scene;
        if (ext == ".prefab" || ext == ".fprefab")
            return AssetType::Prefab;
        if (ext == ".ttf" || ext == ".otf")
            return AssetType::Font;
        return AssetType::None;
    }

    // Asset handle - lightweight reference to an asset
    using AssetHandle = UUID;
    constexpr AssetHandle NullAssetHandle = { 0, 0 };

    // Asset loading state
    enum class AssetLoadState : u8
    {
        Unloaded,
        Loading,
        Loaded,
        Failed
    };

    // Asset metadata stored in .meta files
    struct AssetMetadata
    {
        AssetHandle Handle;
        AssetType   Type = AssetType::None;
        std::string SourcePath;      // relative path from project root
        std::string Name;
        u64         LastModified = 0;
        bool        IsValid = false;
    };
}
