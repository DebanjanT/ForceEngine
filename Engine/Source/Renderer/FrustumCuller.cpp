#include "Force/Renderer/FrustumCuller.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

namespace Force
{
    // Gribb-Hartmann frustum extraction from a combined view-projection matrix.
    Frustum Frustum::FromViewProjection(const glm::mat4& vp)
    {
        Frustum f;
        const auto& m = vp;

        // Left
        f.Planes[0] = glm::vec4(m[0][3] + m[0][0],
                                m[1][3] + m[1][0],
                                m[2][3] + m[2][0],
                                m[3][3] + m[3][0]);
        // Right
        f.Planes[1] = glm::vec4(m[0][3] - m[0][0],
                                m[1][3] - m[1][0],
                                m[2][3] - m[2][0],
                                m[3][3] - m[3][0]);
        // Bottom
        f.Planes[2] = glm::vec4(m[0][3] + m[0][1],
                                m[1][3] + m[1][1],
                                m[2][3] + m[2][1],
                                m[3][3] + m[3][1]);
        // Top
        f.Planes[3] = glm::vec4(m[0][3] - m[0][1],
                                m[1][3] - m[1][1],
                                m[2][3] - m[2][1],
                                m[3][3] - m[3][1]);
        // Near
        f.Planes[4] = glm::vec4(m[0][3] + m[0][2],
                                m[1][3] + m[1][2],
                                m[2][3] + m[2][2],
                                m[3][3] + m[3][2]);
        // Far
        f.Planes[5] = glm::vec4(m[0][3] - m[0][2],
                                m[1][3] - m[1][2],
                                m[2][3] - m[2][2],
                                m[3][3] - m[3][2]);

        // Normalize each plane
        for (auto& p : f.Planes)
        {
            f32 len = glm::length(glm::vec3(p));
            if (len > 1e-6f)
                p /= len;
        }

        return f;
    }

    bool Frustum::TestAABB(const AABB& aabb) const
    {
        for (const auto& plane : Planes)
        {
            glm::vec3 n(plane);
            glm::vec3 positive(
                n.x >= 0.0f ? aabb.Max.x : aabb.Min.x,
                n.y >= 0.0f ? aabb.Max.y : aabb.Min.y,
                n.z >= 0.0f ? aabb.Max.z : aabb.Min.z);

            if (glm::dot(n, positive) + plane.w < 0.0f)
                return false;
        }
        return true;
    }

    bool Frustum::TestSphere(const glm::vec3& c, f32 r) const
    {
        for (const auto& plane : Planes)
        {
            if (glm::dot(glm::vec3(plane), c) + plane.w < -r)
                return false;
        }
        return true;
    }

    bool Frustum::TestPoint(const glm::vec3& p) const
    {
        for (const auto& plane : Planes)
        {
            if (glm::dot(glm::vec3(plane), p) + plane.w < 0.0f)
                return false;
        }
        return true;
    }

    // AABB transform: compute tight world-space AABB from an OBB
    AABB AABB::Transform(const glm::mat4& m) const
    {
        glm::vec3 center  = Center();
        glm::vec3 extents = Extents();

        glm::vec3 newCenter(m * glm::vec4(center, 1.0f));
        glm::vec3 newExtents(0.0f);

        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                newExtents[i] += std::abs(m[j][i]) * extents[j];

        return { newCenter - newExtents, newCenter + newExtents };
    }
}
