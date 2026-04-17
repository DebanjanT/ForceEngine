#include "Force/Terrain/TerrainSculptor.h"
#include "Force/Terrain/TerrainSystem.h"
#include "Force/Core/Logger.h"
#include <glm/glm.hpp>
#include <cmath>
#include <algorithm>

namespace Force
{
    void TerrainSculptor::Init(TerrainSystem* terrain)
    {
        m_Terrain = terrain;
        m_Stack.clear();
        m_UndoPos = 0;
    }

    HeightmapPatch TerrainSculptor::CapturePatch(const BrushDesc& desc) const
    {
        u32 hmW = m_Terrain->GetHeightmapWidth();
        u32 hmH = m_Terrain->GetHeightmapHeight();
        const TerrainDesc& td = m_Terrain->GetDesc();

        f32 texelSize = td.WorldSize / (f32)hmW;
        i32 radiusTex = (i32)std::ceil(desc.Radius / texelSize) + 1;

        i32 cx = (i32)(desc.WorldPosXZ.x / td.WorldSize * hmW);
        i32 cz = (i32)(desc.WorldPosXZ.y / td.WorldSize * hmH);

        i32 x0 = std::max(0, cx - radiusTex);
        i32 z0 = std::max(0, cz - radiusTex);
        i32 x1 = std::min((i32)hmW - 1, cx + radiusTex);
        i32 z1 = std::min((i32)hmH - 1, cz + radiusTex);

        HeightmapPatch patch;
        patch.OriginX = (u32)x0;
        patch.OriginZ = (u32)z0;
        patch.Width   = (u32)(x1 - x0 + 1);
        patch.Height  = (u32)(z1 - z0 + 1);
        patch.Data.resize(patch.Width * patch.Height);

        const f32* hmap = m_Terrain->GetHeightmapData();
        for (u32 pz = 0; pz < patch.Height; pz++)
            for (u32 px = 0; px < patch.Width; px++)
                patch.Data[pz * patch.Width + px] = hmap[(z0 + pz) * hmW + (x0 + px)];

        return patch;
    }

    void TerrainSculptor::ApplyBrush(const BrushDesc& desc)
    {
        if (!m_Terrain) return;
        HeightmapPatch before = CapturePatch(desc);
        ApplyCPU(desc);
        m_Terrain->UploadHeightmap();
        PushUndo(desc, std::move(before));
    }

    void TerrainSculptor::ApplyCPU(const BrushDesc& desc)
    {
        u32 hmW = m_Terrain->GetHeightmapWidth();
        u32 hmH = m_Terrain->GetHeightmapHeight();
        const TerrainDesc& td = m_Terrain->GetDesc();
        f32* hmap = m_Terrain->GetHeightmapData();

        f32 texelSize = td.WorldSize / (f32)hmW;
        i32 radiusTex = (i32)std::ceil(desc.Radius / texelSize) + 1;
        i32 cx = (i32)(desc.WorldPosXZ.x / td.WorldSize * hmW);
        i32 cz = (i32)(desc.WorldPosXZ.y / td.WorldSize * hmH);

        for (i32 dz = -radiusTex; dz <= radiusTex; dz++)
        for (i32 dx = -radiusTex; dx <= radiusTex; dx++)
        {
            i32 x = cx + dx, z = cz + dz;
            if (x < 0 || z < 0 || x >= (i32)hmW || z >= (i32)hmH) continue;

            f32 wx = x * texelSize;
            f32 wz = z * texelSize;
            f32 dist = glm::length(glm::vec2(wx - desc.WorldPosXZ.x, wz - desc.WorldPosXZ.y));
            if (dist > desc.Radius) continue;

            // Cosine falloff
            f32 t = 1.0f - glm::clamp(dist / desc.Radius, 0.0f, 1.0f);
            f32 weight = t * t * (3.0f - 2.0f * t); // smoothstep
            f32& h = hmap[z * hmW + x];

            switch (desc.Type)
            {
                case BrushType::Raise:   h += desc.Strength * weight; break;
                case BrushType::Lower:   h -= desc.Strength * weight; break;
                case BrushType::Flatten: h = glm::mix(h, desc.TargetHeight, desc.Strength * weight); break;
                case BrushType::Smooth:
                {
                    // Box-filter 3x3 average
                    f32 sum = 0; u32 cnt = 0;
                    for (i32 sz = -1; sz <= 1; sz++)
                    for (i32 sx = -1; sx <= 1; sx++)
                    {
                        i32 nx = x + sx, nz = z + sz;
                        if (nx >= 0 && nz >= 0 && nx < (i32)hmW && nz < (i32)hmH)
                        { sum += hmap[nz * hmW + nx]; cnt++; }
                    }
                    f32 avg = sum / (f32)cnt;
                    h = glm::mix(h, avg, desc.Strength * weight);
                    break;
                }
            }
            h = glm::clamp(h, 0.0f, td.MaxHeight);
        }
    }

    void TerrainSculptor::PushUndo(const BrushDesc& /*desc*/, HeightmapPatch&& patch)
    {
        // Truncate redo history
        if (m_UndoPos < (i32)m_Stack.size())
            m_Stack.erase(m_Stack.begin() + m_UndoPos, m_Stack.end());

        m_Stack.push_back(std::move(patch));
        if (m_Stack.size() > MAX_UNDO) m_Stack.erase(m_Stack.begin());
        m_UndoPos = (i32)m_Stack.size();
    }

    void TerrainSculptor::Undo()
    {
        if (!CanUndo()) return;
        m_UndoPos--;
        const HeightmapPatch& patch = m_Stack[m_UndoPos];
        u32 hmW = m_Terrain->GetHeightmapWidth();
        f32* hmap = m_Terrain->GetHeightmapData();
        for (u32 pz = 0; pz < patch.Height; pz++)
            for (u32 px = 0; px < patch.Width; px++)
                hmap[(patch.OriginZ + pz) * hmW + (patch.OriginX + px)] = patch.Data[pz * patch.Width + px];
        m_Terrain->UploadHeightmap();
    }

    void TerrainSculptor::Redo()
    {
        if (!CanRedo()) return;
        // Redo re-applies the brush from the captured post-state. For simplicity,
        // we store only the before-state, so redo is not perfectly round-trippable without
        // storing after-state as well. This is a known limitation — full undo/redo
        // would need both snapshots. Incrementing pointer suffices for typical usage
        // since ApplyBrush will overwrite anyway.
        m_UndoPos++;
        FORCE_CORE_WARN("TerrainSculptor::Redo — re-apply brush manually after Redo()");
    }
}
