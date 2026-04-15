#pragma execution_character_set("utf-8")
#pragma once
#include <SFML/Audio.hpp>
#include <unordered_map>
#include <string>
#include <memory>
#include <optional>




enum class SoundEffect {
    PlaceStone,
    Win,
    Lose,
    Click,
    Error,
    Start,
    TimerWarning
};

class SoundManager {
public:
    SoundManager();

    void loadAll(const std::string& assetsPath);

    void play(SoundEffect effect);

    void playMenuMusic();
    void playGameMusic();
    void stopMusic();


    void setVolume(float volume);
    float getVolume() const;

    
    void setMuted(bool muted);
    bool isMuted() const;

private:
    struct SoundData {
        sf::SoundBuffer buffer;
        std::unique_ptr<sf::Sound> sound;
    };

    std::unordered_map<int, SoundData> m_sounds;
    float m_volume = 70.f;
    bool m_muted = false;

    std::unique_ptr<sf::Music> m_menuMusic;
    std::unique_ptr<sf::Music> m_gameMusic;

    void loadSound(SoundEffect effect, const std::string& path);
};

