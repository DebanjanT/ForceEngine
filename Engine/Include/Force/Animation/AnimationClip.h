#pragma once

#include "Force/Core/Base.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <vector>

namespace Force
{
    // -------------------------------------------------------------------------
    // Keyframe types
    struct PositionKey { f32 Time; glm::vec3 Value; };
    struct RotationKey { f32 Time; glm::quat Value; };
    struct ScaleKey    { f32 Time; glm::vec3 Value; };

    // Per-bone animation data
    struct BoneChannel
    {
        u32                      BoneIndex = 0;
        std::vector<PositionKey> PositionKeys;
        std::vector<RotationKey> RotationKeys;
        std::vector<ScaleKey>    ScaleKeys;

        // Returns TRS combined local matrix at given time
        glm::mat4 Sample(f32 time) const;

    private:
        template<typename Key, typename T>
        static T Interpolate(const std::vector<Key>& keys, f32 time);
    };

    // -------------------------------------------------------------------------
    // Notify fires at a specific time in the clip
    struct AnimNotifyEntry
    {
        f32         Time  = 0.0f;
        std::string Event;        // name dispatched through AnimNotifyDispatcher
    };

    // -------------------------------------------------------------------------
    class AnimationClip
    {
    public:
        const std::string& GetName()           const { return m_Name; }
        f32                GetDuration()       const { return m_Duration; }
        f32                GetTicksPerSecond() const { return m_TicksPerSecond; }
        bool               IsLooping()         const { return m_Looping; }
        void               SetLooping(bool v)        { m_Looping = v; }

        // Sample all bone channels into localPose (caller must size it to bone count)
        void Sample(f32 time, std::vector<glm::mat4>& localPose) const;

        // Collect notifies whose Time falls in (prevTime, currentTime]
        void CollectNotifies(f32 prevTime, f32 currentTime,
                             std::vector<const AnimNotifyEntry*>& out) const;

        const std::vector<BoneChannel>&    GetChannels() const { return m_Channels; }
        const std::vector<AnimNotifyEntry>& GetNotifies() const { return m_Notifies; }

        static Ref<AnimationClip> Create(const std::string& name,
                                         f32 duration, f32 ticksPerSec = 25.0f);

    private:
        friend class AnimationImporter;

        std::string                 m_Name;
        f32                         m_Duration       = 0.0f;
        f32                         m_TicksPerSecond = 25.0f;
        bool                        m_Looping        = true;
        std::vector<BoneChannel>    m_Channels;
        std::vector<AnimNotifyEntry> m_Notifies;
    };
}
