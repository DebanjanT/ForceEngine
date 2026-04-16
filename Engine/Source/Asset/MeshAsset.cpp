#include "Force/Asset/MeshAsset.h"

namespace Force
{
    Ref<Mesh> MeshAsset::GetMeshForLOD(u32 lodIndex) const
    {
        if (lodIndex >= m_LODs.size())
            return m_LODs.empty() ? nullptr : m_LODs.back().MeshData;

        return m_LODs[lodIndex].MeshData;
    }

    Ref<Mesh> MeshAsset::GetMeshForScreenSize(f32 screenSize) const
    {
        if (m_LODs.empty())
            return nullptr;

        // Find the appropriate LOD based on screen size
        for (size_t i = 0; i < m_LODs.size(); ++i)
        {
            if (screenSize >= m_LODs[i].ScreenSize)
            {
                return m_LODs[i].MeshData;
            }
        }

        // Return lowest LOD
        return m_LODs.back().MeshData;
    }
}
