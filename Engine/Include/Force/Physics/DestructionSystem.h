#pragma once

#include "Force/Core/Base.h"
#include <glm/glm.hpp>
#include <vector>
#include <functional>

namespace Force
{
    class Mesh;
    // -------------------------------------------------------------------------
    // Voronoi fracture result — one shard
    struct FractureShardDesc
    {
        Ref<Mesh>   ShardMesh;
        glm::vec3   LocalCentroid = glm::vec3(0.0f);
        f32         Mass          = 1.0f;
    };

    // Fracture manifest built ahead of time (baked, not runtime)
    struct FractureManifest
    {
        std::vector<FractureShardDesc> Shards;
        glm::vec3                      OriginalBoundsMin = glm::vec3(0.0f);
        glm::vec3                      OriginalBoundsMax = glm::vec3(0.0f);
    };

    // -------------------------------------------------------------------------
    // Runtime destruction instance — one destructible object in the world
    struct DestructibleInstance
    {
        u32         Id             = UINT32_MAX;
        f32         Health         = 100.0f;
        f32         BreakThreshold = 50.0f;    // impulse force needed to fracture
        glm::mat4   WorldTransform = glm::mat4(1.0f);
        bool        IsFractured    = false;

        Ref<FractureManifest> Manifest;

        // Called when fracture triggers — game code spawns shard actors
        std::function<void(const DestructibleInstance&)> OnFractured;
    };

    // -------------------------------------------------------------------------
    // API surface — full Voronoi implementation is a future phase
    class DestructionSystem
    {
    public:
        static DestructionSystem& Get();

        // Pre-bake a mesh into N Voronoi shards (offline/load-time)
        static Ref<FractureManifest> FractureMesh(const Ref<Mesh>& mesh, u32 shardCount,
                                                   u32 seed = 42);

        // Register a destructible in the world
        u32  AddDestructible(const DestructibleInstance& instance);
        void RemoveDestructible(u32 id);

        // Apply an impulse at worldPos — if it exceeds BreakThreshold, fracture fires
        void ApplyImpulse(u32 id, const glm::vec3& worldPos, f32 impulseMagnitude);

        void Update(f32 dt);
        void Clear();

    private:
        DestructionSystem() = default;

        std::vector<DestructibleInstance> m_Instances;
        u32                               m_NextId = 0;
    };
}
