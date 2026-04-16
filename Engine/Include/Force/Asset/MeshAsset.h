#pragma once

#include "Force/Asset/Asset.h"
#include "Force/Renderer/Mesh.h"
#include <glm/glm.hpp>

namespace Force
{
    // LOD level for mesh
    struct MeshLOD
    {
        Ref<Mesh>  MeshData;
        f32        ScreenSize = 1.0f;  // Normalized screen size threshold
    };

    class MeshAsset : public Asset
    {
    public:
        FORCE_ASSET_TYPE(Mesh)

        MeshAsset() = default;
        ~MeshAsset() override = default;

        // Get mesh for rendering (uses LOD 0)
        Ref<Mesh> GetMesh() const { return m_LODs.empty() ? nullptr : m_LODs[0].MeshData; }

        // LOD support
        Ref<Mesh> GetMeshForLOD(u32 lodIndex) const;
        Ref<Mesh> GetMeshForScreenSize(f32 screenSize) const;
        u32 GetLODCount() const { return static_cast<u32>(m_LODs.size()); }
        void AddLOD(const MeshLOD& lod) { m_LODs.push_back(lod); }

        // Bounding volume
        const glm::vec3& GetBoundsMin() const { return m_BoundsMin; }
        const glm::vec3& GetBoundsMax() const { return m_BoundsMax; }
        glm::vec3 GetBoundsCenter() const { return (m_BoundsMin + m_BoundsMax) * 0.5f; }
        glm::vec3 GetBoundsExtents() const { return (m_BoundsMax - m_BoundsMin) * 0.5f; }

        // Material slots
        const std::vector<std::string>& GetMaterialSlots() const { return m_MaterialSlots; }
        void SetMaterialSlots(const std::vector<std::string>& slots) { m_MaterialSlots = slots; }

    private:
        friend class MeshImporter;

        std::vector<MeshLOD>    m_LODs;
        std::vector<std::string> m_MaterialSlots; // material names from import

        glm::vec3 m_BoundsMin = glm::vec3(0.0f);
        glm::vec3 m_BoundsMax = glm::vec3(0.0f);
    };
}
