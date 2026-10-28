#include "SoundManager.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace {
constexpr unsigned int sampleRate = 44100;
constexpr float pi = 3.14159265358979323846f;
}

SoundManager::SoundManager() {
    createTone(Effect::Key, 660.f, .045f, .16f);
    createTone(Effect::Submit, 880.f, .12f, .22f);
    createTone(Effect::Click, 420.f, .055f, .18f);
    createTone(Effect::Success, 740.f, .24f, .24f);
    createTone(Effect::Error, 185.f, .26f, .24f);
}

void SoundManager::createTone(Effect effect, float frequency, float duration, float amplitude) {
    const std::size_t sampleCount = static_cast<std::size_t>(sampleRate * duration);
    std::vector<std::int16_t> samples(sampleCount);
    for (std::size_t i = 0; i < sampleCount; ++i) {
        const float progress = static_cast<float>(i) / static_cast<float>(sampleCount);
        const float envelope = std::min(progress * 24.f, 1.f) * std::min((1.f - progress) * 12.f, 1.f);
        const float wave = std::sin(2.f * pi * frequency * static_cast<float>(i) /
                                    static_cast<float>(sampleRate));
        samples[i] = static_cast<std::int16_t>(wave * envelope * amplitude * 32767.f);
    }
    const auto index = static_cast<std::size_t>(effect);
    if (buffers_[index].loadFromSamples(samples.data(), samples.size(), 1, sampleRate,
                                        {sf::SoundChannel::Mono}))
        sounds_[index] = std::make_unique<sf::Sound>(buffers_[index]);
}

void SoundManager::play(Effect effect) {
    if (muted_) return;
    const auto& sound = sounds_[static_cast<std::size_t>(effect)];
    if (sound) {
        sound->setVolume(volume_);
        sound->play();
    }
}

void SoundManager::playKey() { play(Effect::Key); }
void SoundManager::playSubmit() { play(Effect::Submit); }
void SoundManager::playClick() { play(Effect::Click); }
void SoundManager::playSuccess() { play(Effect::Success); }
void SoundManager::playError() { play(Effect::Error); }
void SoundManager::toggleMuted() { muted_ = !muted_; }
bool SoundManager::muted() const { return muted_; }
void SoundManager::setVolume(float volume) { volume_ = std::clamp(volume, 0.f, 100.f); }
float SoundManager::volume() const { return volume_; }
