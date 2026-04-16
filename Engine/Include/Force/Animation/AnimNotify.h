#pragma once

#include "Force/Core/Base.h"
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace Force
{
    // -------------------------------------------------------------------------
    // Base notify event – override Execute() for custom behaviour
    struct AnimNotifyEvent
    {
        std::string Name;
        f32         Time   = 0.0f;

        virtual ~AnimNotifyEvent() = default;
        virtual void Execute() {}
    };

    // Play a 3-D sound at the character position when the animation hits this frame
    struct AnimNotify_PlaySound : public AnimNotifyEvent
    {
        std::string AudioClipPath;
        f32         Volume     = 1.0f;
        f32         Pitch      = 1.0f;
        bool        Is3D       = true;

        void Execute() override;   // calls AudioEngine::PlayOneShot
    };

    // Spawn a named VFX at a named socket
    struct AnimNotify_SpawnVFX : public AnimNotifyEvent
    {
        std::string VFXName;
        std::string SocketName;

        void Execute() override {}
    };

    // Arbitrary lambda callback
    struct AnimNotify_Script : public AnimNotifyEvent
    {
        std::function<void()> Callback;
        void Execute() override { if (Callback) Callback(); }
    };

    // -------------------------------------------------------------------------
    // Global dispatcher: map event names → handlers, fire on animation notify
    class AnimNotifyDispatcher
    {
    public:
        static AnimNotifyDispatcher& Get();

        using Handler = std::function<void(const AnimNotifyEvent&)>;
        void RegisterHandler(const std::string& eventName, Handler handler);
        void Fire(const AnimNotifyEvent& event);

        // Fire by name only (no payload)
        void FireByName(const std::string& eventName);

        void Clear();

    private:
        AnimNotifyDispatcher() = default;
        std::unordered_map<std::string, std::vector<Handler>> m_Handlers;
    };
}
