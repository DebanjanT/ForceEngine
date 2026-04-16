#pragma once

#include "Force/Core/Base.h"
#include "Force/Audio/AudioSource.h"
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include <vector>


namespace Force
{
    // -------------------------------------------------------------------------
    struct ReverbZone
    {
        glm::vec3 Position;
        f32       Radius     = 10.0f;
        f32       Decay      = 1.5f;
        f32       Diffusion  = 1.0f;
        f32       Density    = 1.0f;
        f32       Gain       = 0.32f;
        f32       GainHF     = 0.89f;
    };

    // -------------------------------------------------------------------------
    // Ambient music layer – cross-fades between zones
    struct AmbientLayer
    {
        std::string ClipPath;
        f32         Volume     = 1.0f;
        f32         FadeTime   = 2.0f;   // seconds to cross-fade
        bool        Loop       = true;
    };

    // -------------------------------------------------------------------------
    class AudioEngine
    {
    public:
        static AudioEngine& Get();

        void Init();
        void Shutdown();

        // Call every frame to clean up finished one-shot sources and update reverb
        void Update();

        // Listener (typically set from camera / character each frame)
        void SetListenerPosition   (const glm::vec3& pos);
        void SetListenerOrientation(const glm::vec3& forward, const glm::vec3& up);
        void SetListenerVelocity   (const glm::vec3& vel);
        void SetMasterVolume       (f32 volume);

        // Audio clip cache
        Ref<AudioClip> LoadClip   (const std::string& path);
        Ref<AudioClip> GetClip    (const std::string& name);
        void           UnloadClip (const std::string& name);

        // 3-D reverb zones (requires EFX extension)
        void AddReverbZone(const ReverbZone& zone);
        void UpdateReverbZones(const glm::vec3& listenerPos);

        // Ambient atmosphere management
        void PlayAmbientLayer (const AmbientLayer& layer);
        void StopAmbientLayer (const std::string& clipPath);
        void StopAllAmbient   ();

        // Fire-and-forget 3-D sound (engine manages source lifetime)
        void PlayOneShot(const Ref<AudioClip>& clip, const glm::vec3& position,
                         f32 volume = 1.0f, f32 pitch = 1.0f);

        bool IsInitialized() const { return m_Initialized; }

        void* GetDevice()  const { return m_Device;  }
        void* GetContext() const { return m_Context; }

    private:
        AudioEngine() = default;
        void InitEFX();
        void CleanupOneShotSources();
        void TickAmbientLayers(f32 dt);

    private:
        void* m_Device  = nullptr;   // ALCdevice*
        void* m_Context = nullptr;   // ALCcontext*
        bool               m_Initialized  = false;
        bool               m_EFXAvailable = false;

        std::unordered_map<std::string, Ref<AudioClip>> m_Clips;

        std::vector<ALuint>     m_OneShotSources;
        std::vector<ReverbZone> m_ReverbZones;

        // Ambient layer state
        struct ActiveAmbient
        {
            AmbientLayer   Layer;
            AudioSource    Source;
            f32            CurrentVolume = 0.0f;
            bool           FadingOut     = false;
        };
        std::vector<ActiveAmbient> m_AmbientLayers;

        f32 m_MasterVolume = 1.0f;
    };
}
