#pragma once
#include "Force/Core/Base.h"
#include "Force/Renderer/Texture.h"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>

namespace Force
{
    static constexpr u32 MAX_TERRAIN_LAYERS = 8;

    struct TerrainLayer
    {
        Ref<Texture2D> Albedo;
        Ref<Texture2D> Normal;
        f32            Tiling    = 4.0f;
        bool           Triplanar = false;
        std::string    Name;
    };

    struct PaintBrushDesc
    {
        glm::vec2 WorldPosXZ = {};
        f32       Radius     = 8.0f;
        f32       Strength   = 0.5f;
        u32       LayerIndex = 0;
    };

    class TerrainPainter
    {
    public:
        void Init(u32 blendMapWidth, u32 blendMapHeight);
        void Shutdown();

        u32  AddLayer(const TerrainLayer& layer);
        void SetLayer(u32 index, const TerrainLayer& layer);
        const TerrainLayer& GetLayer(u32 index) const { return m_Layers[index]; }
        u32  GetLayerCount() const { return static_cast<u32>(m_Layers.size()); }

        void PaintBrush(const PaintBrushDesc& desc, f32 terrainWorldSize);
        void UploadBlendMaps();

        // Blend map 0 = layers 0-3 (RGBA), blend map 1 = layers 4-7 (RGBA)
        VkImageView GetBlendMap0View()   const { return m_Map0View; }
        VkImageView GetBlendMap1View()   const { return m_Map1View; }
        VkSampler   GetBlendMapSampler() const { return m_Sampler; }

    private:
        void CreateImages();
        void NormalizeWeights(u32 x, u32 z);

    private:
        u32 m_W = 0, m_H = 0;

        std::vector<TerrainLayer> m_Layers;
        std::vector<u8> m_Map0; // layers 0-3
        std::vector<u8> m_Map1; // layers 4-7

        VkImage       m_Map0Image = VK_NULL_HANDLE;
        VmaAllocation m_Map0Alloc = VK_NULL_HANDLE;
        VkImageView   m_Map0View  = VK_NULL_HANDLE;
        VkImage       m_Map1Image = VK_NULL_HANDLE;
        VmaAllocation m_Map1Alloc = VK_NULL_HANDLE;
        VkImageView   m_Map1View  = VK_NULL_HANDLE;
        VkSampler     m_Sampler   = VK_NULL_HANDLE;
    };
}
