#pragma execution_character_set("utf-8")
#pragma once
#include "Board.h"
#include <vector>

class Replay {
public:
    Replay();

    
    void loadHistory(const std::vector<Move>& history);

    
    void nextMove();
    void prevMove();
    void goToStart();
    void goToEnd();

    
    void setAutoPlay(bool enable, float speedSeconds = 1.0f);
    void update(float dt); 

    const Board& getReplayBoard() const;

    int getCurrentStep() const;
    int getTotalSteps() const;
    bool isAutoPlaying() const;

private:
    std::vector<Move> m_history;
    Board m_board;
    int m_currentStep;
    
    bool m_autoPlay;
    float m_autoPlaySpeed;
    float m_autoPlayTimer;

    void updateBoardToCurrentStep();
};

