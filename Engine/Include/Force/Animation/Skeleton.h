#pragma once

#include "Force/Core/Base.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vulkan/vulkan.h>
#include <array>
#include <string>
#include <vector>
#include <unordered_map>

namespace Force
{
    struct Bone
    {
        std::string Name;
        i32         ParentIndex       = -1;
        glm::mat4   LocalBindPose     = glm::mat4(1.0f);  // local → parent in bind pose
        glm::mat4   InverseBindMatrix = glm::mat4(1.0f);  // model-space → bone-space
    };

    class Skeleton
    {
    public:
        void AddBone(const Bone& bone);
        i32  GetBoneIndex(const std::string& name) const;

        const std::vector<Bone>& GetBones()     const { return m_Bones; }
        u32                      GetBoneCount() const { return static_cast<u32>(m_Bones.size()); }

        // Compute global skinning palette from per-bone local transforms.
        // outPalette[i] = globalMatrix[i] * inverseBindMatrix[i]
        void ComputeSkinningMatrices(const std::vector<glm::mat4>& localPose,
                                     std::vector<glm::mat4>&       outPalette) const;

        static Ref<Skeleton> Create();

    private:
        std::vector<Bone>                    m_Bones;
        std::unordered_map<std::string, i32> m_BoneNameToIndex;
    };

    // Vertex layout for GPU-skinned meshes (4 bone influences)
    struct SkinnedVertex
    {
        glm::vec3  Position;
        glm::vec3  Normal;
        glm::vec2  TexCoord;
        glm::vec3  Tangent;
        glm::ivec4 BoneIndices = { 0, 0, 0, 0 };
        glm::vec4  BoneWeights = { 1.0f, 0.0f, 0.0f, 0.0f };

        static VkVertexInputBindingDescription                       GetBindingDescription();
        static std::array<VkVertexInputAttributeDescription, 6>      GetAttributeDescriptions();
    };
}
