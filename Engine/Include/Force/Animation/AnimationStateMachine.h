#pragma once

#include "Force/Core/Base.h"
#include "Force/Animation/AnimationClip.h"
#include <glm/glm.hpp>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace Force
{
    class Skeleton;

    // -------------------------------------------------------------------------
    // 1D blend tree: lerp between clips keyed on a float parameter
    struct BlendTree1D
    {
        std::string ParamName;
        struct Entry { f32 Threshold; Ref<AnimationClip> Clip; };
        std::vector<Entry> Entries; // must be sorted by Threshold

        void Sample(f32 param, f32 time, u32 boneCount,
                    std::vector<glm::mat4>& out) const;
    };

    // 2D blend tree: bilinear blend on a vec2 parameter space
    struct BlendTree2D
    {
        std::string ParamX, ParamY;
        struct Entry { glm::vec2 Point; Ref<AnimationClip> Clip; };
        std::vector<Entry> Entries;

        void Sample(glm::vec2 param, f32 time, u32 boneCount,
                    std::vector<glm::mat4>& out) const;
    };

    // -------------------------------------------------------------------------
    struct AnimState
    {
        std::string        Name;
        Ref<AnimationClip> Clip;           // nullptr if using a blend tree
        Ref<BlendTree1D>   BlendTree1;
        Ref<BlendTree2D>   BlendTree2;
        f32                Speed      = 1.0f;
        bool               Loop       = true;
    };

    struct AnimTransition
    {
        std::string            From;
        std::string            To;
        std::function<bool()>  Condition;
        f32                    BlendDuration = 0.2f;
        bool                   HasExitTime   = false;
        f32                    ExitTime      = 1.0f;  // normalized [0,1]
    };

    // -------------------------------------------------------------------------
    class AnimationStateMachine
    {
    public:
        void Init(const Ref<Skeleton>& skeleton);

        void AddState(const AnimState& state);
        void AddTransition(const AnimTransition& transition);

        // Parameter setters used by transition conditions
        void SetFloat(const std::string& name, f32 value);
        void SetBool (const std::string& name, bool value);
        void SetVec2 (const std::string& name, glm::vec2 value);
        f32       GetFloat(const std::string& name) const;
        bool      GetBool (const std::string& name) const;
        glm::vec2 GetVec2 (const std::string& name) const;

        // Advance time, evaluate transitions, fire notifies, compute final pose
        void Update(f32 dt);

        const std::string&            GetCurrentStateName()    const { return m_CurrentState; }
        const std::vector<glm::mat4>& GetSkinningMatrices()   const { return m_SkinningPalette; }
        const std::vector<glm::mat4>& GetLocalPose()          const { return m_FinalPose; }

        // Returns root-motion delta accumulated since last call (resets after)
        glm::vec3 ConsumeRootMotion();

        // Called for every AnimNotifyEntry that fires during Update
        void SetNotifyCallback(std::function<void(const std::string&)> cb)
        { m_NotifyCallback = std::move(cb); }

        static Ref<AnimationStateMachine> Create(const Ref<Skeleton>& skeleton);

        static void BlendPoses(const std::vector<glm::mat4>& a,
                               const std::vector<glm::mat4>& b,
                               f32 t, std::vector<glm::mat4>& out);

    private:
        void EvaluateState(const AnimState& state, f32 time,
                           std::vector<glm::mat4>& out) const;
        void FireNotifies(f32 prevTime, f32 curTime, const AnimState& state);

    private:
        Ref<Skeleton> m_Skeleton;
        u32           m_BoneCount = 0;

        std::unordered_map<std::string, AnimState> m_States;
        std::vector<AnimTransition>                m_Transitions;

        std::string m_CurrentState;
        std::string m_BlendToState;
        f32         m_CurrentTime   = 0.0f;
        f32         m_BlendTime     = 0.0f;
        f32         m_BlendDuration = 0.0f;
        bool        m_IsBlending    = false;

        std::unordered_map<std::string, f32>      m_Floats;
        std::unordered_map<std::string, bool>     m_Bools;
        std::unordered_map<std::string, glm::vec2> m_Vec2s;

        std::vector<glm::mat4> m_FinalPose;
        std::vector<glm::mat4> m_SkinningPalette;
        glm::vec3              m_RootMotionDelta = glm::vec3(0.0f);

        std::function<void(const std::string&)> m_NotifyCallback;
    };
}
