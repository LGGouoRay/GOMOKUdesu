#pragma execution_character_set("utf-8")
#pragma once
#include <SFML/Graphics.hpp>




class Timer {
public:
    Timer();

    void setEnabled(bool enabled);
    bool isEnabled() const;

    void setTimeLimit(float seconds);
    float getTimeLimit() const;

    
    void reset();

    
    void pause();
    void resume();
    bool isPaused() const;

    
    void update();

    
    float getRemainingSeconds() const;

    bool isTimeUp() const;

    std::string getTimeString() const;

    
    bool isWarning() const;

private:
    bool m_enabled = false;
    bool m_paused = false;
    float m_timeLimit = 30.f;
    sf::Clock m_clock;
    float m_elapsed = 0.f;     
    float m_pauseAccum = 0.f;  
};

