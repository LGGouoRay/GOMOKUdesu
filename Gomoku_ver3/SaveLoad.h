#pragma execution_character_set("utf-8")
#pragma once
#include "Board.h"
#include <string>
#include <vector>





struct GameSaveData {
    std::array<std::array<int, BOARD_SIZE>, BOARD_SIZE> grid;
    std::vector<Move> moveHistory;
    std::string saveTime;
    int currentTurn;       
    bool timerEnabled;
    float timerLimit;
    bool undoEnabled;
    bool aiEnabled;
};

class SaveLoad {
public:
    static bool saveGame(const std::string& filename, const GameSaveData& data);
    
    static bool loadGame(const std::string& filename, GameSaveData& data);

    static std::string exportRecord(const std::vector<Move>& history);

    static void ensureSaveDirectory(const std::string& dir = "saves");
};

