#include "Force/Audio/AudioEngine.h"

#ifdef FORCE_AUDIO_OPENAL
#include "Force/Core/Logger.h"
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/efx.h>
#include <algorithm>
#include <cstring>

// Cast void* members to the concrete ALC types used in this translation unit
#define ALC_DEVICE(p)  static_cast<ALCdevice*>(p)
#define ALC_CONTEXT(p) static_cast<ALCcontext*>(p)

namespace Force
{
    AudioEngine& AudioEngine::Get()
    {
        static AudioEngine instance;
        return instance;
    }

    void AudioEngine::Init()
    {
        if (m_Initialized) return;

        m_Device = alcOpenDevice(nullptr);
        if (!m_Device)
        {
            FORCE_CORE_ERROR("AudioEngine: failed to open default OpenAL device");
            return;
        }

        m_Context = alcCreateContext(ALC_DEVICE(m_Device), nullptr);
        if (!m_Context || alcMakeContextCurrent(ALC_CONTEXT(m_Context)) == ALC_FALSE)
        {
            FORCE_CORE_ERROR("AudioEngine: failed to create/activate OpenAL context");
            alcCloseDevice(ALC_DEVICE(m_Device));
            m_Device = nullptr;
            return;
        }

        // Distance model: inverse with clamped distance
        alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);

        InitEFX();

        m_Initialized = true;
        FORCE_CORE_INFO("AudioEngine initialized (EFX: {})", m_EFXAvailable ? "yes" : "no");
    }

    void AudioEngine::InitEFX()
    {
        m_EFXAvailable = alcIsExtensionPresent(ALC_DEVICE(m_Device), "ALC_EXT_EFX") == ALC_TRUE;
    }

    void AudioEngine::Shutdown()
    {
        if (!m_Initialized) return;

        StopAllAmbient();

        for (ALuint src : m_OneShotSources)
        {
            alSourceStop(src);
            alDeleteSources(1, &src);
        }
        m_OneShotSources.clear();

        m_Clips.clear();

        alcMakeContextCurrent(nullptr);
        if (m_Context) { alcDestroyContext(ALC_CONTEXT(m_Context)); m_Context = nullptr; }
        if (m_Device)  { alcCloseDevice(ALC_DEVICE(m_Device));      m_Device  = nullptr; }

        m_Initialized = false;
        FORCE_CORE_INFO("AudioEngine shutdown");
    }

    void AudioEngine::Update()
    {
        if (!m_Initialized) return;
        CleanupOneShotSources();
        // Ambient fade logic would need a dt; called as needed from TickAmbientLayers
    }

    // -------------------------------------------------------------------------
    // Listener

    void AudioEngine::SetListenerPosition(const glm::vec3& pos)
    {
        alListener3f(AL_POSITION, pos.x, pos.y, pos.z);
    }

    void AudioEngine::SetListenerOrientation(const glm::vec3& forward, const glm::vec3& up)
    {
        float orientation[6] = {
            forward.x, forward.y, forward.z,
            up.x,      up.y,      up.z
        };
        alListenerfv(AL_ORIENTATION, orientation);
    }

    void AudioEngine::SetListenerVelocity(const glm::vec3& vel)
    {
        alListener3f(AL_VELOCITY, vel.x, vel.y, vel.z);
    }

    void AudioEngine::SetMasterVolume(f32 volume)
    {
        m_MasterVolume = volume;
        alListenerf(AL_GAIN, volume);
    }

    // -------------------------------------------------------------------------
    // Clip management

    Ref<AudioClip> AudioEngine::LoadClip(const std::string& path)
    {
        auto it = m_Clips.find(path);
        if (it != m_Clips.end()) return it->second;

        auto clip = CreateRef<AudioClip>();
        if (clip->LoadFromFile(path))
        {
            m_Clips[path] = clip;
            return clip;
        }
        return nullptr;
    }

    Ref<AudioClip> AudioEngine::GetClip(const std::string& name)
    {
        auto it = m_Clips.find(name);
        return (it != m_Clips.end()) ? it->second : nullptr;
    }

    void AudioEngine::UnloadClip(const std::string& name)
    {
        m_Clips.erase(name);
    }

    // -------------------------------------------------------------------------
    // One-shot

    void AudioEngine::PlayOneShot(const Ref<AudioClip>& clip, const glm::vec3& position,
                                   f32 volume, f32 pitch)
    {
        if (!m_Initialized || !clip || !clip->IsLoaded()) return;

        ALuint src;
        alGenSources(1, &src);
        alSourcei (src, AL_BUFFER,  static_cast<ALint>(clip->GetBuffer()));
        alSourcef (src, AL_GAIN,    volume);
        alSourcef (src, AL_PITCH,   pitch);
        alSource3f(src, AL_POSITION, position.x, position.y, position.z);
        alSourcei (src, AL_LOOPING, AL_FALSE);
        alSourcePlay(src);

        m_OneShotSources.push_back(src);
    }

    void AudioEngine::CleanupOneShotSources()
    {
        m_OneShotSources.erase(
            std::remove_if(m_OneShotSources.begin(), m_OneShotSources.end(),
                [](ALuint src) {
                    ALint state;
                    alGetSourcei(src, AL_SOURCE_STATE, &state);
                    if (state != AL_PLAYING)
                    {
                        alDeleteSources(1, &src);
                        return true;
                    }
                    return false;
                }),
            m_OneShotSources.end());
    }

    // -------------------------------------------------------------------------
    // Reverb zones

    void AudioEngine::AddReverbZone(const ReverbZone& zone)
    {
        m_ReverbZones.push_back(zone);
    }

    void AudioEngine::UpdateReverbZones(const glm::vec3& listenerPos)
    {
        // Find the nearest active zone (EFX-based — stub, wired up when EFX is available)
        if (!m_EFXAvailable) return;

        for (const auto& zone : m_ReverbZones)
        {
            f32 dist = glm::length(listenerPos - zone.Position);
            if (dist < zone.Radius)
            {
                // Apply reverb via EFX — full EFX wiring kept separate to avoid header bloat
                (void)zone;
                break;
            }
        }
    }

    // -------------------------------------------------------------------------
    // Ambient layers

    void AudioEngine::PlayAmbientLayer(const AmbientLayer& layer)
    {
        auto clip = LoadClip(layer.ClipPath);
        if (!clip) return;

        ActiveAmbient amb;
        amb.Layer = layer;
        amb.Source.Create();
        amb.Source.SetClip(clip);
        amb.Source.SetSpatial(false);  // 2-D, no positioning
        amb.Source.SetLooping(layer.Loop);
        amb.Source.SetVolume(0.0f);    // fade in
        amb.Source.Play();
        m_AmbientLayers.push_back(std::move(amb));
    }

    void AudioEngine::StopAmbientLayer(const std::string& clipPath)
    {
        for (auto& amb : m_AmbientLayers)
        {
            if (amb.Layer.ClipPath == clipPath)
                amb.FadingOut = true;
        }
    }

    void AudioEngine::StopAllAmbient()
    {
        for (auto& amb : m_AmbientLayers)
        {
            amb.Source.Stop();
            amb.Source.Destroy();
        }
        m_AmbientLayers.clear();
    }

    void AudioEngine::TickAmbientLayers(f32 dt)
    {
        for (auto& amb : m_AmbientLayers)
        {
            f32 target = amb.FadingOut ? 0.0f : amb.Layer.Volume * m_MasterVolume;
            f32 speed  = (amb.Layer.FadeTime > 1e-6f) ? dt / amb.Layer.FadeTime : 1.0f;
            amb.CurrentVolume = glm::mix(amb.CurrentVolume, target, speed);
            amb.Source.SetVolume(amb.CurrentVolume);
        }

        m_AmbientLayers.erase(
            std::remove_if(m_AmbientLayers.begin(), m_AmbientLayers.end(),
                [](const ActiveAmbient& a) {
                    return a.FadingOut && a.CurrentVolume < 0.001f;
                }),
            m_AmbientLayers.end());
    }
}

#endif // FORCE_AUDIO_OPENAL
