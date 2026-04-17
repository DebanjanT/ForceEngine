#pragma once
#include "Force/Core/Base.h"
#include "Force/Renderer/VulkanBuffer.h"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <array>

namespace Force
{
    static constexpr u32 TERRAIN_MAX_LOD    = 5;
    static constexpr u32 TERRAIN_CHUNK_VERTS = 17; // 16 quads + 1

    struct TerrainVertex
    {
        glm::vec3 Position;
        glm::vec3 Normal;
        glm::vec2 UV;
    };

    struct TerrainDesc
    {
        std::string HeightmapPath;
        f32 WorldSize        = 1024.0f;
        f32 MaxHeight        = 128.0f;
        u32 ChunkCountX      = 16;
        u32 ChunkCountZ      = 16;
        f32 LODDistanceBase  = 128.0f;
    };

    struct TerrainChunk
    {
        glm::ivec2 GridPos   = {};
        f32        ChunkSize = 0.0f;
        u32        CurrentLOD = 0;

        VulkanBuffer VertexBuf;
        std::array<u32, TERRAIN_MAX_LOD> IndexCounts = {};

        bool Loaded  = false;
        bool Visible = false;
    };

    class TerrainSystem
    {
    public:
        static TerrainSystem& Get();

        void Init(const TerrainDesc& desc);
        void Shutdown();

        void Update(const glm::vec3& cameraPos);
        void Draw(VkCommandBuffer cmd, VkPipelineLayout layout);

        f32* GetHeightmapData()     { return m_Heightmap.data(); }
        u32  GetHeightmapWidth()    const { return m_HmapW; }
        u32  GetHeightmapHeight()   const { return m_HmapH; }
        void UploadHeightmap();

        VkImageView GetHeightmapView()   const { return m_HmapView; }
        VkSampler   GetHeightmapSampler() const { return m_HmapSampler; }
        const TerrainDesc& GetDesc()     const { return m_Desc; }
        bool IsInitialized()             const { return m_Initialized; }

    private:
        void LoadHeightmap();
        void GenerateChunks();
        void GenerateChunkMesh(TerrainChunk& chunk);
        u32  SelectLOD(const TerrainChunk& chunk, const glm::vec3& camPos) const;
        f32  SampleHeight(f32 wx, f32 wz) const;
        glm::vec3 ComputeNormal(f32 wx, f32 wz) const;
        void CreateHeightmapImage();

    private:
        TerrainDesc m_Desc;
        bool        m_Initialized = false;

        std::vector<f32> m_Heightmap;
        u32              m_HmapW = 0, m_HmapH = 0;

        VkImage       m_HmapImage   = VK_NULL_HANDLE;
        VmaAllocation m_HmapAlloc   = VK_NULL_HANDLE;
        VkImageView   m_HmapView    = VK_NULL_HANDLE;
        VkSampler     m_HmapSampler = VK_NULL_HANDLE;

        // Shared LOD index buffers (same topology for every chunk)
        std::array<VulkanBuffer, TERRAIN_MAX_LOD> m_SharedIndexBufs;
        std::array<u32,          TERRAIN_MAX_LOD> m_SharedIndexCounts = {};

        std::vector<TerrainChunk> m_Chunks;
    };
}
