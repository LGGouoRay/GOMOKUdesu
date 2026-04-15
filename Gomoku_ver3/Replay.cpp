#pragma execution_character_set("utf-8")
#include "Replay.h"
#include <algorithm>





Replay::Replay() : m_currentStep(0), m_autoPlay(false), m_autoPlaySpeed(1.0f), m_autoPlayTimer(0.0f) {
}

void Replay::loadHistory(const std::vector<Move>& history) {
    m_history = history;
    goToStart();
}

void Replay::nextMove() {
    if (m_currentStep < static_cast<int>(m_history.size())) {
        m_currentStep++;
        updateBoardToCurrentStep();
    } else {
        m_autoPlay = false; 
    }
}

void Replay::prevMove() {
    if (m_currentStep > 0) {
        m_currentStep--;
        updateBoardToCurrentStep();
    }
}

void Replay::goToStart() {
    m_currentStep = 0;
    updateBoardToCurrentStep();
}

void Replay::goToEnd() {
    m_currentStep = static_cast<int>(m_history.size());
    updateBoardToCurrentStep();
}

void Replay::setAutoPlay(bool enable, float speedSeconds) {
    m_autoPlay = enable;
    m_autoPlaySpeed = speedSeconds;
    m_autoPlayTimer = 0.0f;
}

void Replay::update(float dt) {
    if (!m_autoPlay) return;
    
    m_autoPlayTimer += dt;
    if (m_autoPlayTimer >= m_autoPlaySpeed) {
        m_autoPlayTimer = 0.0f;
        nextMove();
    }
}

const Board& Replay::getReplayBoard() const {
    return m_board;
}

int Replay::getCurrentStep() const {
    return m_currentStep;
}

int Replay::getTotalSteps() const {
    return static_cast<int>(m_history.size());
}

bool Replay::isAutoPlaying() const {
    return m_autoPlay;
}

void Replay::updateBoardToCurrentStep() {
    m_board.reset();
    for (int i = 0; i < m_currentStep; ++i) {
        const auto& m = m_history[i];
        m_board.placeStone(m.row, m.col, m.color);
    }
}

