#pragma once

#include "Force/Core/Base.h"
#include <glm/glm.hpp>
#include <string>

// Use the concrete typedefs from AL/al.h without pulling in the full header here
#ifndef FORCE_AL_TYPES_DEFINED
#define FORCE_AL_TYPES_DEFINED
using ALuint = unsigned int;
using ALenum = int;
#endif

namespace Force
{
    // -------------------------------------------------------------------------
    class AudioClip
    {
    public:
        AudioClip() = default;
        ~AudioClip();

        bool LoadFromFile(const std::string& path);
        void Destroy();

        ALuint             GetBuffer()   const { return m_Buffer; }
        const std::string& GetName()     const { return m_Name; }
        f32                GetDuration() const { return m_Duration; }
        bool               IsLoaded()   const { return m_Buffer != 0; }

    private:
        ALuint      m_Buffer   = 0;
        std::string m_Name;
        f32         m_Duration = 0.0f;
    };

    // -------------------------------------------------------------------------
    class AudioSource
    {
    public:
        AudioSource();
        ~AudioSource();

        void Create();
        void Destroy();

        void SetClip(const Ref<AudioClip>& clip);
        void Play();
        void Stop();
        void Pause();
        bool IsPlaying() const;

        void SetPosition   (const glm::vec3& pos);
        void SetVelocity   (const glm::vec3& vel);
        void SetVolume     (f32 volume);
        void SetPitch      (f32 pitch);
        void SetLooping    (bool loop);
        void SetMinDistance(f32 dist);
        void SetMaxDistance(f32 dist);
        void SetSpatial    (bool spatial);   // false = 2D (ignores position)

        ALuint GetSource() const { return m_Source; }

    private:
        ALuint         m_Source = 0;
        Ref<AudioClip> m_Clip;
    };
}
