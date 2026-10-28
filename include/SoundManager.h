#pragma once

#include <SFML/Audio.hpp>

#include <array>
#include <memory>

class SoundManager {
public:
    SoundManager();

    void playKey();
    void playSubmit();
    void playClick();
    void playSuccess();
    void playError();
    void toggleMuted();
    bool muted() const;
    void setVolume(float volume);
    float volume() const;

private:
    enum class Effect { Key, Submit, Click, Success, Error, Count };

    void play(Effect effect);
    void createTone(Effect effect, float frequency, float duration, float amplitude);

    std::array<sf::SoundBuffer, static_cast<std::size_t>(Effect::Count)> buffers_;
    std::array<std::unique_ptr<sf::Sound>, static_cast<std::size_t>(Effect::Count)> sounds_;
    bool muted_ = false;
    float volume_ = 42.f;
};
