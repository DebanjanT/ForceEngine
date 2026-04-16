#include "Force/Animation/AnimationStateMachine.h"
#include "Force/Animation/Skeleton.h"
#include "Force/Core/Logger.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cassert>

namespace Force
{
    // -------------------------------------------------------------------------
    // BlendTree1D

    void BlendTree1D::Sample(f32 param, f32 time, u32 boneCount,
                              std::vector<glm::mat4>& out) const
    {
        if (Entries.empty()) return;
        out.assign(boneCount, glm::mat4(1.0f));

        if (Entries.size() == 1)
        {
            Entries[0].Clip->Sample(time, out);
            return;
        }

        // Clamp to range
        param = std::clamp(param, Entries.front().Threshold, Entries.back().Threshold);

        for (u32 i = 0; i + 1 < static_cast<u32>(Entries.size()); ++i)
        {
            if (param <= Entries[i + 1].Threshold)
            {
                f32 range = Entries[i + 1].Threshold - Entries[i].Threshold;
                f32 t     = (range > 1e-6f) ?
                            (param - Entries[i].Threshold) / range : 0.0f;

                std::vector<glm::mat4> poseA(boneCount, glm::mat4(1.0f));
                std::vector<glm::mat4> poseB(boneCount, glm::mat4(1.0f));
                Entries[i].Clip->Sample(time, poseA);
                Entries[i + 1].Clip->Sample(time, poseB);
                AnimationStateMachine::BlendPoses(poseA, poseB, t, out);
                return;
            }
        }

        Entries.back().Clip->Sample(time, out);
    }

    // -------------------------------------------------------------------------
    // BlendTree2D – simple nearest-2 weighted blend

    void BlendTree2D::Sample(glm::vec2 param, f32 time, u32 boneCount,
                              std::vector<glm::mat4>& out) const
    {
        if (Entries.empty()) return;
        out.assign(boneCount, glm::mat4(1.0f));

        if (Entries.size() == 1)
        {
            Entries[0].Clip->Sample(time, out);
            return;
        }

        // Find two nearest entries and blend by inverse distance
        int   best1 = 0, best2 = 1;
        float d1 = glm::length(Entries[0].Point - param);
        float d2 = glm::length(Entries[1].Point - param);
        if (d1 > d2) { std::swap(best1, best2); std::swap(d1, d2); }

        for (u32 i = 2; i < static_cast<u32>(Entries.size()); ++i)
        {
            f32 d = glm::length(Entries[i].Point - param);
            if (d < d1)       { best2 = best1; d2 = d1; best1 = i; d1 = d; }
            else if (d < d2)  { best2 = i;     d2 = d; }
        }

        f32 total = d1 + d2;
        f32 t     = (total > 1e-6f) ? d1 / total : 0.5f;

        std::vector<glm::mat4> pA(boneCount, glm::mat4(1.0f));
        std::vector<glm::mat4> pB(boneCount, glm::mat4(1.0f));
        Entries[best1].Clip->Sample(time, pA);
        Entries[best2].Clip->Sample(time, pB);
        AnimationStateMachine::BlendPoses(pA, pB, t, out);
    }

    // -------------------------------------------------------------------------
    // AnimationStateMachine

    void AnimationStateMachine::Init(const Ref<Skeleton>& skeleton)
    {
        m_Skeleton  = skeleton;
        m_BoneCount = skeleton ? skeleton->GetBoneCount() : 0;
        m_FinalPose.assign(m_BoneCount, glm::mat4(1.0f));
        m_SkinningPalette.assign(m_BoneCount, glm::mat4(1.0f));
    }

    void AnimationStateMachine::AddState(const AnimState& state)
    {
        m_States[state.Name] = state;
        if (m_CurrentState.empty())
            m_CurrentState = state.Name;
    }

    void AnimationStateMachine::AddTransition(const AnimTransition& t)
    {
        m_Transitions.push_back(t);
    }

    void AnimationStateMachine::SetFloat(const std::string& n, f32 v)   { m_Floats[n] = v; }
    void AnimationStateMachine::SetBool (const std::string& n, bool v)  { m_Bools[n]  = v; }
    void AnimationStateMachine::SetVec2 (const std::string& n, glm::vec2 v) { m_Vec2s[n] = v; }
    f32       AnimationStateMachine::GetFloat(const std::string& n) const
    { auto it = m_Floats.find(n); return it != m_Floats.end() ? it->second : 0.0f; }
    bool      AnimationStateMachine::GetBool(const std::string& n) const
    { auto it = m_Bools.find(n);  return it != m_Bools.end()  ? it->second : false; }
    glm::vec2 AnimationStateMachine::GetVec2(const std::string& n) const
    { auto it = m_Vec2s.find(n);  return it != m_Vec2s.end()  ? it->second : glm::vec2(0); }

    void AnimationStateMachine::Update(f32 dt)
    {
        if (m_CurrentState.empty() || m_States.empty()) return;

        const AnimState& current = m_States[m_CurrentState];
        f32 duration = current.Clip ? current.Clip->GetDuration() : 0.0f;

        f32 prevTime   = m_CurrentTime;
        m_CurrentTime += dt * current.Speed;

        if (current.Loop && duration > 0.0f)
            m_CurrentTime = std::fmod(m_CurrentTime, duration);
        else if (!current.Loop && m_CurrentTime > duration)
            m_CurrentTime = duration;

        // Fire notifies for this frame's window
        FireNotifies(prevTime, m_CurrentTime, current);

        // Check transitions
        if (!m_IsBlending)
        {
            for (const auto& trans : m_Transitions)
            {
                if (trans.From != m_CurrentState) continue;
                bool condOk = trans.Condition && trans.Condition();
                bool exitOk = !trans.HasExitTime ||
                              (duration > 0.0f && (m_CurrentTime / duration) >= trans.ExitTime);
                if (condOk && exitOk)
                {
                    m_BlendToState  = trans.To;
                    m_BlendDuration = trans.BlendDuration;
                    m_BlendTime     = 0.0f;
                    m_IsBlending    = true;
                    break;
                }
            }
        }

        // Evaluate pose
        if (m_IsBlending)
        {
            m_BlendTime += dt;
            f32 t = (m_BlendDuration > 1e-6f) ? m_BlendTime / m_BlendDuration : 1.0f;

            std::vector<glm::mat4> poseA(m_BoneCount, glm::mat4(1.0f));
            std::vector<glm::mat4> poseB(m_BoneCount, glm::mat4(1.0f));
            EvaluateState(current, m_CurrentTime, poseA);
            EvaluateState(m_States[m_BlendToState], 0.0f, poseB);
            BlendPoses(poseA, poseB, glm::clamp(t, 0.0f, 1.0f), m_FinalPose);

            if (t >= 1.0f)
            {
                m_CurrentState = m_BlendToState;
                m_BlendToState = {};
                m_IsBlending   = false;
                m_CurrentTime  = 0.0f;
            }
        }
        else
        {
            EvaluateState(current, m_CurrentTime, m_FinalPose);
        }

        // Compute skinning palette
        if (m_Skeleton)
            m_Skeleton->ComputeSkinningMatrices(m_FinalPose, m_SkinningPalette);
    }

    void AnimationStateMachine::EvaluateState(const AnimState& state, f32 time,
                                               std::vector<glm::mat4>& out) const
    {
        out.assign(m_BoneCount, glm::mat4(1.0f));
        if (state.BlendTree1)
        {
            f32 p = GetFloat(state.BlendTree1->ParamName);
            state.BlendTree1->Sample(p, time, m_BoneCount, out);
        }
        else if (state.BlendTree2)
        {
            glm::vec2 p = GetVec2(state.BlendTree2->ParamX);
            state.BlendTree2->Sample(p, time, m_BoneCount, out);
        }
        else if (state.Clip)
        {
            state.Clip->Sample(time, out);
        }
    }

    void AnimationStateMachine::BlendPoses(const std::vector<glm::mat4>& a,
                                            const std::vector<glm::mat4>& b,
                                            f32 t, std::vector<glm::mat4>& out)
    {
        out.resize(a.size());
        for (u32 i = 0; i < static_cast<u32>(a.size()); ++i)
        {
            // Per-component lerp (TRS decomposition kept implicit via matrix lerp)
            for (int c = 0; c < 4; ++c)
                for (int r = 0; r < 4; ++r)
                    out[i][c][r] = glm::mix(a[i][c][r], b[i][c][r], t);
        }
    }

    void AnimationStateMachine::FireNotifies(f32 prevTime, f32 curTime,
                                              const AnimState& state)
    {
        if (!state.Clip || !m_NotifyCallback) return;
        std::vector<const AnimNotifyEntry*> notifies;
        state.Clip->CollectNotifies(prevTime, curTime, notifies);
        for (const auto* n : notifies)
            m_NotifyCallback(n->Event);
    }

    glm::vec3 AnimationStateMachine::ConsumeRootMotion()
    {
        glm::vec3 delta = m_RootMotionDelta;
        m_RootMotionDelta = glm::vec3(0.0f);
        return delta;
    }

    Ref<AnimationStateMachine> AnimationStateMachine::Create(const Ref<Skeleton>& skeleton)
    {
        auto asm_ = CreateRef<AnimationStateMachine>();
        asm_->Init(skeleton);
        return asm_;
    }
}
