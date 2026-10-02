#include "af/app/AudioSystem.h"

#include <cmath>
#include <cstring>
#include <cstdio>
#include <algorithm>

namespace af {

namespace {
constexpr int SampleRate = 44100;
constexpr float TwoPi = 6.28318530718f;

// Fast deterministic LCG PRNG for noise generation
float NextNoise(uint32_t& state) {
    state = state * 1664525u + 1013904223u;
    return static_cast<float>(state >> 16) / 65535.0f;
}
}  // namespace

AudioSystem::AudioSystem() = default;

AudioSystem::~AudioSystem() {
    Shutdown();
}

bool AudioSystem::Init() {
    if (initialized_) return true;

    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
        std::fprintf(stderr, "AstraForge Audio: SDL_InitSubSystem failed: %s\n", SDL_GetError());
        return false;
    }

    SDL_AudioSpec desired = {};
    desired.freq = SampleRate;
    desired.format = AUDIO_F32SYS;
    desired.channels = 1;
    desired.samples = 512;
    desired.callback = AudioCallback;
    desired.userdata = this;

    SDL_AudioSpec obtained = {};
    deviceId_ = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, 0);
    if (deviceId_ == 0) {
        std::fprintf(stderr, "AstraForge Audio: failed to open audio device: %s\n", SDL_GetError());
        return false;
    }

    SDL_PauseAudioDevice(deviceId_, 0);  // Unpause
    initialized_ = true;
    return true;
}

void AudioSystem::Shutdown() {
    if (!initialized_) return;
    if (deviceId_ != 0) {
        SDL_CloseAudioDevice(deviceId_);
        deviceId_ = 0;
    }
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    initialized_ = false;
}

void AudioSystem::Play(SoundEffect effect) {
    if (!initialized_ || muted_) return;

    SDL_LockAudioDevice(deviceId_);
    for (int i = 0; i < MaxVoices; ++i) {
        if (!voices_[i].active) {
            voices_[i].type = effect;
            voices_[i].progress = 0.0f;
            voices_[i].active = true;
            voices_[i].noiseState = 12345u + static_cast<uint32_t>(i * 777);

            switch (effect) {
                case SoundEffect::Shoot:     voices_[i].duration = 0.12f; break;
                case SoundEffect::Hit:       voices_[i].duration = 0.08f; break;
                case SoundEffect::Explosion: voices_[i].duration = 0.35f; break;
                case SoundEffect::Jump:      voices_[i].duration = 0.14f; break;
                case SoundEffect::Pickup:    voices_[i].duration = 0.18f; break;
            }
            break;
        }
    }
    SDL_UnlockAudioDevice(deviceId_);
}

void AudioSystem::SetVolume(float volume) {
    volume_ = std::clamp(volume, 0.0f, 1.0f);
}

void AudioSystem::SetMuted(bool muted) {
    muted_ = muted;
}

void AudioSystem::AudioCallback(void* userdata, Uint8* stream, int len) {
    auto* self = static_cast<AudioSystem*>(userdata);
    std::memset(stream, 0, len);
    const int numSamples = len / static_cast<int>(sizeof(float));
    self->GenerateAudio(reinterpret_cast<float*>(stream), numSamples);
}

void AudioSystem::GenerateAudio(float* output, int numSamples) {
    if (muted_ || volume_ <= 0.0001f) return;

    constexpr float dt = 1.0f / static_cast<float>(SampleRate);

    for (int s = 0; s < numSamples; ++s) {
        float sampleMix = 0.0f;

        for (int i = 0; i < MaxVoices; ++i) {
            Voice& v = voices_[i];
            if (!v.active) continue;

            const float t = v.progress;
            const float dur = v.duration;
            float out = 0.0f;

            switch (v.type) {
                case SoundEffect::Shoot: {
                    // Laser pitch drop from 950 Hz down to 250 Hz
                    const float freq = 950.0f - (t / dur) * 700.0f;
                    const float phase = t * freq * TwoPi;
                    const float env = (1.0f - t / dur);
                    // Square-like pulse with soft decay
                    out = (std::sin(phase) > 0.0f ? 0.35f : -0.35f) * env;
                    break;
                }
                case SoundEffect::Hit: {
                    // Quick crunchy impact
                    const float freq = 160.0f;
                    const float phase = t * freq * TwoPi;
                    const float noise = NextNoise(v.noiseState) * 2.0f - 1.0f;
                    const float env = (1.0f - t / dur);
                    out = (std::sin(phase) * 0.4f + noise * 0.4f) * env;
                    break;
                }
                case SoundEffect::Explosion: {
                    // Decaying white noise boom
                    const float noise = NextNoise(v.noiseState) * 2.0f - 1.0f;
                    const float env = std::exp(-t * 9.0f);
                    out = noise * env * 0.6f;
                    break;
                }
                case SoundEffect::Jump: {
                    // Rising sweep 200 Hz -> 500 Hz
                    const float freq = 200.0f + (t / dur) * 300.0f;
                    const float phase = t * freq * TwoPi;
                    const float env = (1.0f - t / dur);
                    out = std::sin(phase) * env * 0.4f;
                    break;
                }
                case SoundEffect::Pickup: {
                    // Arpeggio: 587 Hz then 880 Hz
                    const float freq = (t < dur * 0.5f) ? 587.0f : 880.0f;
                    const float phase = t * freq * TwoPi;
                    const float env = (1.0f - t / dur);
                    out = std::sin(phase) * env * 0.4f;
                    break;
                }
            }

            sampleMix += out;
            v.progress += dt;
            if (v.progress >= v.duration) {
                v.active = false;
            }
        }

        output[s] = std::clamp(sampleMix * volume_, -1.0f, 1.0f);
    }
}

}  // namespace af
