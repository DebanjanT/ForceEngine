#pragma once
#include "Force/Core/Base.h"
#include <glm/glm.hpp>

namespace Force
{
    struct alignas(16) DirectionalLight
    {
        glm::vec4 Direction    = glm::vec4(0.0f, -1.0f, 0.0f, 0.0f); // w unused
        glm::vec4 Color        = glm::vec4(1.0f, 1.0f,  1.0f, 1.0f); // w = intensity
        u32       CascadeCount = 4;
        f32       ShadowBias   = 0.001f;
        f32       _pad[2]      = {};
    };

    struct alignas(16) PointLight
    {
        glm::vec4 Position    = glm::vec4(0.0f); // w = radius
        glm::vec4 Color       = glm::vec4(1.0f); // w = intensity
        u32       CastShadows = 0;
        f32       _pad[3]     = {};
    };

    struct alignas(16) SpotLight
    {
        glm::vec4 Position  = glm::vec4(0.0f);               // w = range
        glm::vec4 Direction = glm::vec4(0.0f, -1.0f, 0.0f, 0.866f); // w = cos(inner)
        glm::vec4 Color     = glm::vec4(1.0f);               // w = intensity
        f32       CosOuter  = 0.7071f;
        u32       CastShadows = 0;
        f32       _pad[2]   = {};
    };
}
