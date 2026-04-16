#include "Force/Animation/Skeleton.h"
#include "Force/Core/Logger.h"
#include <glm/gtc/matrix_transform.hpp>

namespace Force
{
    void Skeleton::AddBone(const Bone& bone)
    {
        i32 idx = static_cast<i32>(m_Bones.size());
        m_BoneNameToIndex[bone.Name] = idx;
        m_Bones.push_back(bone);
    }

    i32 Skeleton::GetBoneIndex(const std::string& name) const
    {
        auto it = m_BoneNameToIndex.find(name);
        return (it != m_BoneNameToIndex.end()) ? it->second : -1;
    }

    void Skeleton::ComputeSkinningMatrices(const std::vector<glm::mat4>& localPose,
                                            std::vector<glm::mat4>&       outPalette) const
    {
        const u32 n = static_cast<u32>(m_Bones.size());
        outPalette.resize(n, glm::mat4(1.0f));

        std::vector<glm::mat4> globalPose(n, glm::mat4(1.0f));

        for (u32 i = 0; i < n; ++i)
        {
            const Bone& bone   = m_Bones[i];
            const glm::mat4& local = (i < localPose.size()) ? localPose[i]
                                                              : bone.LocalBindPose;
            if (bone.ParentIndex < 0)
                globalPose[i] = local;
            else
                globalPose[i] = globalPose[bone.ParentIndex] * local;

            outPalette[i] = globalPose[i] * bone.InverseBindMatrix;
        }
    }

    Ref<Skeleton> Skeleton::Create()
    {
        return CreateRef<Skeleton>();
    }

    // -------------------------------------------------------------------------
    // SkinnedVertex layout
    VkVertexInputBindingDescription SkinnedVertex::GetBindingDescription()
    {
        VkVertexInputBindingDescription d{};
        d.binding   = 0;
        d.stride    = sizeof(SkinnedVertex);
        d.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return d;
    }

    std::array<VkVertexInputAttributeDescription, 6> SkinnedVertex::GetAttributeDescriptions()
    {
        std::array<VkVertexInputAttributeDescription, 6> a{};
        // Position
        a[0] = { 0, 0, VK_FORMAT_R32G32B32_SFLOAT,    (u32)offsetof(SkinnedVertex, Position)    };
        // Normal
        a[1] = { 1, 0, VK_FORMAT_R32G32B32_SFLOAT,    (u32)offsetof(SkinnedVertex, Normal)      };
        // TexCoord
        a[2] = { 2, 0, VK_FORMAT_R32G32_SFLOAT,       (u32)offsetof(SkinnedVertex, TexCoord)    };
        // Tangent
        a[3] = { 3, 0, VK_FORMAT_R32G32B32_SFLOAT,    (u32)offsetof(SkinnedVertex, Tangent)     };
        // BoneIndices
        a[4] = { 4, 0, VK_FORMAT_R32G32B32A32_SINT,   (u32)offsetof(SkinnedVertex, BoneIndices) };
        // BoneWeights
        a[5] = { 5, 0, VK_FORMAT_R32G32B32A32_SFLOAT, (u32)offsetof(SkinnedVertex, BoneWeights) };
        return a;
    }
}
