#pragma execution_character_set("utf-8")
#include "Timer.h"
#include <sstream>
#include <iomanip>
#include <algorithm>




Timer::Timer() {
    m_clock.restart();
}

void Timer::setEnabled(bool enabled) {
    m_enabled = enabled;
    if (enabled) reset();
}

bool Timer::isEnabled() const { return m_enabled; }

void Timer::setTimeLimit(float seconds) {
    m_timeLimit = std::max(5.f, seconds);
}

float Timer::getTimeLimit() const { return m_timeLimit; }

void Timer::reset() {
    m_clock.restart();
    m_elapsed = 0.f;
    m_pauseAccum = 0.f;
    m_paused = false;
}

void Timer::pause() {
    if (!m_paused) {
        m_paused = true;
        m_elapsed += m_clock.restart().asSeconds();
    }
}

void Timer::resume() {
    if (m_paused) {
        m_paused = false;
        m_clock.restart();
    }
}

bool Timer::isPaused() const { return m_paused; }

void Timer::update() {
}

float Timer::getRemainingSeconds() const {
    if (!m_enabled) return m_timeLimit;
    float total = m_elapsed;
    if (!m_paused) {
        total += m_clock.getElapsedTime().asSeconds();
    }
    return std::max(0.f, m_timeLimit - total);
}

bool Timer::isTimeUp() const {
    if (!m_enabled) return false;
    return getRemainingSeconds() <= 0.f;
}

std::string Timer::getTimeString() const {
    float remaining = getRemainingSeconds();
    int totalSec = static_cast<int>(remaining);
    int min = totalSec / 60;
    int sec = totalSec % 60;
    std::ostringstream ss;
    ss << std::setw(2) << std::setfill('0') << min << ":"
       << std::setw(2) << std::setfill('0') << sec;
    return ss.str();
}

bool Timer::isWarning() const {
    return m_enabled && getRemainingSeconds() <= 5.f && getRemainingSeconds() > 0.f;
}

