#include "Force/Physics/DestructionSystem.h"
#include "Force/Core/Logger.h"
#include "Force/Renderer/Mesh.h"
#include <algorithm>

namespace Force
{
    DestructionSystem& DestructionSystem::Get()
    {
        static DestructionSystem instance;
        return instance;
    }

    // Voronoi fracture — full implementation is a future phase.
    // For now, return a manifest with a single shard matching the original mesh.
    Ref<FractureManifest> DestructionSystem::FractureMesh(const Ref<Mesh>& mesh,
                                                            u32 /*shardCount*/,
                                                            u32 /*seed*/)
    {
        FORCE_CORE_WARN("DestructionSystem::FractureMesh — Voronoi not yet implemented, "
                        "returning single-shard stub");
        auto manifest = CreateRef<FractureManifest>();
        if (mesh)
        {
            manifest->OriginalBoundsMin = mesh->GetBoundingBoxMin();
            manifest->OriginalBoundsMax = mesh->GetBoundingBoxMax();

            FractureShardDesc shard;
            shard.ShardMesh    = mesh;
            shard.LocalCentroid = (mesh->GetBoundingBoxMin() + mesh->GetBoundingBoxMax()) * 0.5f;
            shard.Mass          = 1.0f;
            manifest->Shards.push_back(std::move(shard));
        }
        return manifest;
    }

    u32 DestructionSystem::AddDestructible(const DestructibleInstance& instance)
    {
        DestructibleInstance inst = instance;
        inst.Id = m_NextId++;
        m_Instances.push_back(std::move(inst));
        return m_Instances.back().Id;
    }

    void DestructionSystem::RemoveDestructible(u32 id)
    {
        m_Instances.erase(
            std::remove_if(m_Instances.begin(), m_Instances.end(),
                [id](const DestructibleInstance& i){ return i.Id == id; }),
            m_Instances.end());
    }

    void DestructionSystem::ApplyImpulse(u32 id, const glm::vec3& /*worldPos*/,
                                          f32 impulseMagnitude)
    {
        for (auto& inst : m_Instances)
        {
            if (inst.Id != id || inst.IsFractured) continue;
            inst.Health -= impulseMagnitude;
            if (inst.Health <= inst.BreakThreshold)
            {
                inst.IsFractured = true;
                if (inst.OnFractured)
                    inst.OnFractured(inst);
                FORCE_CORE_INFO("DestructionSystem: object {} fractured", id);
            }
            break;
        }
    }

    void DestructionSystem::Update(f32 /*dt*/)
    {
        // Remove permanently fractured instances that have no further role
        m_Instances.erase(
            std::remove_if(m_Instances.begin(), m_Instances.end(),
                [](const DestructibleInstance& i){ return i.IsFractured && !i.OnFractured; }),
            m_Instances.end());
    }

    void DestructionSystem::Clear()
    {
        m_Instances.clear();
    }
}
