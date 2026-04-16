#include "Force/Animation/AnimNotify.h"
#include "Force/Audio/AudioEngine.h"
#include "Force/Core/Logger.h"

namespace Force
{
    // -------------------------------------------------------------------------
    // AnimNotify_PlaySound

    void AnimNotify_PlaySound::Execute()
    {
        auto& audio = AudioEngine::Get();
        if (!audio.IsInitialized()) return;

        auto clip = audio.LoadClip(AudioClipPath);
        if (!clip) return;

        // Play at world origin by default; caller should set position externally
        audio.PlayOneShot(clip, glm::vec3(0.0f), Volume, Pitch);
    }

    // -------------------------------------------------------------------------
    // AnimNotifyDispatcher

    AnimNotifyDispatcher& AnimNotifyDispatcher::Get()
    {
        static AnimNotifyDispatcher instance;
        return instance;
    }

    void AnimNotifyDispatcher::RegisterHandler(const std::string& eventName, Handler handler)
    {
        m_Handlers[eventName].push_back(std::move(handler));
    }

    void AnimNotifyDispatcher::Fire(const AnimNotifyEvent& event)
    {
        auto it = m_Handlers.find(event.Name);
        if (it != m_Handlers.end())
            for (auto& h : it->second)
                h(event);
    }

    void AnimNotifyDispatcher::FireByName(const std::string& eventName)
    {
        AnimNotifyEvent dummy;
        dummy.Name = eventName;
        Fire(dummy);
    }

    void AnimNotifyDispatcher::Clear()
    {
        m_Handlers.clear();
    }
}
