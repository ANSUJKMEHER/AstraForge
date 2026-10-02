#pragma once

#include <cstdint>
#include <SDL.h>

namespace af {

enum class SoundEffect {
    Shoot,
    Hit,
    Explosion,
    Jump,
    Pickup
};

class AudioSystem {
public:
    AudioSystem();
    ~AudioSystem();

    AudioSystem(const AudioSystem&) = delete;
    AudioSystem& operator=(const AudioSystem&) = delete;

    bool Init();
    void Shutdown();

    void Play(SoundEffect effect);
    void SetVolume(float volume);  // 0.0f to 1.0f
    float GetVolume() const { return volume_; }
    void SetMuted(bool muted);
    bool IsMuted() const { return muted_; }

private:
    struct Voice {
        SoundEffect type = SoundEffect::Shoot;
        float progress = 0.0f;  // current time in seconds
        float duration = 0.0f;  // total lifetime in seconds
        bool active = false;
        uint32_t noiseState = 12345u;
    };

    static constexpr int MaxVoices = 16;
    Voice voices_[MaxVoices] = {};
    SDL_AudioDeviceID deviceId_ = 0;
    float volume_ = 0.6f;
    bool muted_ = false;
    bool initialized_ = false;

    static void AudioCallback(void* userdata, Uint8* stream, int len);
    void GenerateAudio(float* output, int numSamples);
};

}  // namespace af
