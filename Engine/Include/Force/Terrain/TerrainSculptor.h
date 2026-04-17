#pragma once
#include "Force/Core/Base.h"
#include <glm/glm.hpp>
#include <vector>

namespace Force
{
    class TerrainSystem;

    enum class BrushType : u32
    {
        Raise   = 0,
        Lower   = 1,
        Smooth  = 2,
        Flatten = 3,
    };

    struct BrushDesc
    {
        BrushType Type         = BrushType::Raise;
        glm::vec2 WorldPosXZ   = {};
        f32       Radius       = 8.0f;
        f32       Strength     = 0.5f;
        f32       TargetHeight = 0.0f; // Flatten only
    };

    struct HeightmapPatch
    {
        u32              OriginX, OriginZ;
        u32              Width,   Height;
        std::vector<f32> Data; // snapshot before stroke
    };

    class TerrainSculptor
    {
    public:
        void Init(TerrainSystem* terrain);

        void ApplyBrush(const BrushDesc& desc);
        void Undo();
        void Redo();

        bool CanUndo() const { return m_UndoPos > 0; }
        bool CanRedo() const { return m_UndoPos < static_cast<i32>(m_Stack.size()); }

    private:
        void ApplyCPU(const BrushDesc& desc);
        void PushUndo(const BrushDesc& desc, HeightmapPatch&& patch);
        HeightmapPatch CapturePatch(const BrushDesc& desc) const;

        static constexpr u32 MAX_UNDO = 64;
        TerrainSystem*              m_Terrain = nullptr;
        std::vector<HeightmapPatch> m_Stack;
        i32                         m_UndoPos = 0;
    };
}
