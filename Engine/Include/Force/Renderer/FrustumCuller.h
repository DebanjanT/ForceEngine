#pragma once

#include "Force/Core/Base.h"
#include <glm/glm.hpp>
#include <array>

namespace Force
{
    struct AABB
    {
        glm::vec3 Min = glm::vec3(0.0f);
        glm::vec3 Max = glm::vec3(0.0f);

        glm::vec3 Center()  const { return (Min + Max) * 0.5f; }
        glm::vec3 Extents() const { return (Max - Min) * 0.5f; }

        AABB Transform(const glm::mat4& m) const;
    };

    struct Frustum
    {
        // 0=left, 1=right, 2=bottom, 3=top, 4=near, 5=far  (xyz=normal, w=d)
        std::array<glm::vec4, 6> Planes{};

        static Frustum FromViewProjection(const glm::mat4& viewProj);

        bool TestAABB(const AABB& aabb)              const;
        bool TestSphere(const glm::vec3& c, f32 r)   const;
        bool TestPoint(const glm::vec3& p)            const;
    };
}
