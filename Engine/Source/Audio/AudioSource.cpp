#include "Force/Audio/AudioSource.h"

#ifdef FORCE_AUDIO_OPENAL
#include "Force/Core/Logger.h"
#include <AL/al.h>
#include <AL/alc.h>

#include <fstream>
#include <vector>
#include <cstring>

namespace Force
{
    // -------------------------------------------------------------------------
    // AudioClip

    AudioClip::~AudioClip()
    {
        Destroy();
    }

    bool AudioClip::LoadFromFile(const std::string& path)
    {
        Destroy();

        m_Name = path;

        // Detect format by extension
        std::string ext = path.substr(path.find_last_of('.'));
        for (auto& c : ext) c = static_cast<char>(::tolower(c));

        if (ext == ".wav")
        {
            std::ifstream f(path, std::ios::binary);
            if (!f) { FORCE_CORE_ERROR("AudioClip: cannot open '{}'", path); return false; }

            auto read32 = [&]() { u32 v = 0; f.read(reinterpret_cast<char*>(&v), 4); return v; };
            auto read16 = [&]() { u16 v = 0; f.read(reinterpret_cast<char*>(&v), 2); return v; };

            // RIFF header
            if (read32() != 0x46464952u) { FORCE_CORE_ERROR("AudioClip: not a RIFF file"); return false; }
            read32(); // file size
            if (read32() != 0x45564157u) { FORCE_CORE_ERROR("AudioClip: not a WAVE file"); return false; }

            u16 channels = 0, bitsPerSample = 0;
            u32 sampleRate = 0;
            std::vector<u8> pcmData;

            while (f)
            {
                u32 chunkId   = read32();
                u32 chunkSize = read32();
                if (!f) break;

                if (chunkId == 0x20746D66u) // "fmt "
                {
                    read16();           // audioFormat (1=PCM)
                    channels      = read16();
                    sampleRate    = read32();
                    read32();           // byteRate
                    read16();           // blockAlign
                    bitsPerSample = read16();
                    if (chunkSize > 16) f.seekg(chunkSize - 16, std::ios::cur);
                }
                else if (chunkId == 0x61746164u) // "data"
                {
                    pcmData.resize(chunkSize);
                    f.read(reinterpret_cast<char*>(pcmData.data()), chunkSize);
                    break;
                }
                else
                {
                    f.seekg(chunkSize, std::ios::cur);
                }
            }

            if (pcmData.empty()) { FORCE_CORE_ERROR("AudioClip: no data chunk in '{}'", path); return false; }

            ALenum format;
            if      (channels == 1 && bitsPerSample == 8)  format = AL_FORMAT_MONO8;
            else if (channels == 1 && bitsPerSample == 16) format = AL_FORMAT_MONO16;
            else if (channels == 2 && bitsPerSample == 8)  format = AL_FORMAT_STEREO8;
            else                                            format = AL_FORMAT_STEREO16;

            alGenBuffers(1, &m_Buffer);
            alBufferData(m_Buffer, format, pcmData.data(),
                         static_cast<ALsizei>(pcmData.size()),
                         static_cast<ALsizei>(sampleRate));

            m_Duration = static_cast<f32>(pcmData.size()) /
                         static_cast<f32>(sampleRate * channels * (bitsPerSample / 8));

            FORCE_CORE_INFO("AudioClip: loaded WAV '{}' ({:.2f}s)", path, m_Duration);
            return true;
        }

        FORCE_CORE_ERROR("AudioClip: unsupported format '{}'", ext);
        return false;
    }

    void AudioClip::Destroy()
    {
        if (m_Buffer)
        {
            alDeleteBuffers(1, &m_Buffer);
            m_Buffer = 0;
        }
    }

    // -------------------------------------------------------------------------
    // AudioSource

    AudioSource::AudioSource() = default;
    AudioSource::~AudioSource() { Destroy(); }

    void AudioSource::Create()
    {
        alGenSources(1, &m_Source);
        alSourcef(m_Source, AL_REFERENCE_DISTANCE, 1.0f);
        alSourcef(m_Source, AL_MAX_DISTANCE,       500.0f);
        alSourcei(m_Source, AL_SOURCE_RELATIVE,    AL_FALSE);
    }

    void AudioSource::Destroy()
    {
        if (m_Source)
        {
            alSourceStop(m_Source);
            alDeleteSources(1, &m_Source);
            m_Source = 0;
        }
    }

    void AudioSource::SetClip(const Ref<AudioClip>& clip)
    {
        m_Clip = clip;
        if (m_Source && clip && clip->IsLoaded())
            alSourcei(m_Source, AL_BUFFER, static_cast<ALint>(clip->GetBuffer()));
    }

    void AudioSource::Play()
    {
        if (m_Source) alSourcePlay(m_Source);
    }

    void AudioSource::Stop()
    {
        if (m_Source) alSourceStop(m_Source);
    }

    void AudioSource::Pause()
    {
        if (m_Source) alSourcePause(m_Source);
    }

    bool AudioSource::IsPlaying() const
    {
        if (!m_Source) return false;
        ALint state;
        alGetSourcei(m_Source, AL_SOURCE_STATE, &state);
        return state == AL_PLAYING;
    }

    void AudioSource::SetPosition(const glm::vec3& pos)
    {
        if (m_Source) alSource3f(m_Source, AL_POSITION, pos.x, pos.y, pos.z);
    }

    void AudioSource::SetVelocity(const glm::vec3& vel)
    {
        if (m_Source) alSource3f(m_Source, AL_VELOCITY, vel.x, vel.y, vel.z);
    }

    void AudioSource::SetVolume(f32 v)
    {
        if (m_Source) alSourcef(m_Source, AL_GAIN, v);
    }

    void AudioSource::SetPitch(f32 p)
    {
        if (m_Source) alSourcef(m_Source, AL_PITCH, p);
    }

    void AudioSource::SetLooping(bool loop)
    {
        if (m_Source) alSourcei(m_Source, AL_LOOPING, loop ? AL_TRUE : AL_FALSE);
    }

    void AudioSource::SetMinDistance(f32 d)
    {
        if (m_Source) alSourcef(m_Source, AL_REFERENCE_DISTANCE, d);
    }

    void AudioSource::SetMaxDistance(f32 d)
    {
        if (m_Source) alSourcef(m_Source, AL_MAX_DISTANCE, d);
    }

    void AudioSource::SetSpatial(bool spatial)
    {
        if (m_Source)
            alSourcei(m_Source, AL_SOURCE_RELATIVE, spatial ? AL_FALSE : AL_TRUE);
    }
}

#endif // FORCE_AUDIO_OPENAL
