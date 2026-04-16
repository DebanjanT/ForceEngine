#include "Force/Animation/AnimationClip.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

namespace Force
{
    // -------------------------------------------------------------------------
    // BoneChannel helpers

    static u32 FindKeyIndex(const auto& keys, f32 time)
    {
        for (u32 i = 0; i + 1 < static_cast<u32>(keys.size()); ++i)
            if (time < keys[i + 1].Time)
                return i;
        return static_cast<u32>(keys.size()) - 2;
    }

    glm::mat4 BoneChannel::Sample(f32 time) const
    {
        glm::vec3 position(0.0f);
        glm::quat rotation(1.0f, 0.0f, 0.0f, 0.0f);
        glm::vec3 scale(1.0f);

        // Position
        if (!PositionKeys.empty())
        {
            if (PositionKeys.size() == 1 || time <= PositionKeys.front().Time)
            {
                position = PositionKeys.front().Value;
            }
            else if (time >= PositionKeys.back().Time)
            {
                position = PositionKeys.back().Value;
            }
            else
            {
                u32 i = FindKeyIndex(PositionKeys, time);
                f32 t = (time - PositionKeys[i].Time) /
                        (PositionKeys[i + 1].Time - PositionKeys[i].Time);
                position = glm::mix(PositionKeys[i].Value, PositionKeys[i + 1].Value, t);
            }
        }

        // Rotation
        if (!RotationKeys.empty())
        {
            if (RotationKeys.size() == 1 || time <= RotationKeys.front().Time)
            {
                rotation = RotationKeys.front().Value;
            }
            else if (time >= RotationKeys.back().Time)
            {
                rotation = RotationKeys.back().Value;
            }
            else
            {
                u32 i = FindKeyIndex(RotationKeys, time);
                f32 t = (time - RotationKeys[i].Time) /
                        (RotationKeys[i + 1].Time - RotationKeys[i].Time);
                rotation = glm::slerp(RotationKeys[i].Value, RotationKeys[i + 1].Value, t);
                rotation = glm::normalize(rotation);
            }
        }

        // Scale
        if (!ScaleKeys.empty())
        {
            if (ScaleKeys.size() == 1 || time <= ScaleKeys.front().Time)
            {
                scale = ScaleKeys.front().Value;
            }
            else if (time >= ScaleKeys.back().Time)
            {
                scale = ScaleKeys.back().Value;
            }
            else
            {
                u32 i = FindKeyIndex(ScaleKeys, time);
                f32 t = (time - ScaleKeys[i].Time) /
                        (ScaleKeys[i + 1].Time - ScaleKeys[i].Time);
                scale = glm::mix(ScaleKeys[i].Value, ScaleKeys[i + 1].Value, t);
            }
        }

        glm::mat4 mat = glm::translate(glm::mat4(1.0f), position);
        mat *= glm::mat4_cast(rotation);
        mat  = glm::scale(mat, scale);
        return mat;
    }

    // -------------------------------------------------------------------------
    void AnimationClip::Sample(f32 time, std::vector<glm::mat4>& localPose) const
    {
        for (const auto& ch : m_Channels)
        {
            if (ch.BoneIndex < static_cast<u32>(localPose.size()))
                localPose[ch.BoneIndex] = ch.Sample(time);
        }
    }

    void AnimationClip::CollectNotifies(f32 prevTime, f32 currentTime,
                                         std::vector<const AnimNotifyEntry*>& out) const
    {
        for (const auto& n : m_Notifies)
        {
            if (n.Time > prevTime && n.Time <= currentTime)
                out.push_back(&n);
        }
    }

    Ref<AnimationClip> AnimationClip::Create(const std::string& name,
                                              f32 duration, f32 ticksPerSec)
    {
        auto clip              = CreateRef<AnimationClip>();
        clip->m_Name           = name;
        clip->m_Duration       = duration;
        clip->m_TicksPerSecond = ticksPerSec;
        return clip;
    }
}
