#pragma execution_character_set("utf-8")
#include "SaveLoad.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>

void SaveLoad::ensureSaveDirectory(const std::string& dir) {
    std::filesystem::create_directories(dir);
}

bool SaveLoad::saveGame(const std::string& filename, const GameSaveData& data) {
    ensureSaveDirectory("saves");
    std::ofstream file(filename);
    if (!file.is_open()) return false;

    
    file << "GOMOKU_SAVE_V2\n";
    
    // Write timestamp
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    struct tm timeinfo;
    localtime_s(&timeinfo, &now_c); // Windows safe wrapper
    file << "time=" << std::put_time(&timeinfo, "%Y-%m-%d %H:%M:%S") << "\n";

    file << "turn=" << data.currentTurn << "\n";
    file << "timer_enabled=" << (data.timerEnabled ? 1 : 0) << "\n";
    file << "timer_limit=" << data.timerLimit << "\n";
    file << "undo_enabled=" << (data.undoEnabled ? 1 : 0) << "\n";
    file << "ai_enabled=" << (data.aiEnabled ? 1 : 0) << "\n";

    
    file << "BOARD\n";
    for (int r = 0; r < BOARD_SIZE; ++r) {
        for (int c = 0; c < BOARD_SIZE; ++c) {
            file << data.grid[r][c];
            if (c < BOARD_SIZE - 1) file << ",";
        }
        file << "\n";
    }

    
    file << "HISTORY\n";
    file << data.moveHistory.size() << "\n";
    for (const auto& m : data.moveHistory) {
        file << m.row << "," << m.col << "," << static_cast<int>(m.color) << "\n";
    }

    file << "END\n";
    file.close();
    std::cout << "[SaveLoad] Game saved to: " << filename << "\n";
    return true;
}

bool SaveLoad::loadGame(const std::string& filename, GameSaveData& data) {
    std::ifstream file(filename);
    if (!file.is_open()) return false;

    std::string line;
    
    std::getline(file, line);
    bool isV2 = (line == "GOMOKU_SAVE_V2");
    if (line != "GOMOKU_SAVE_V1" && !isV2) return false;

    auto readString = [&](const std::string& key) -> std::string {
        std::getline(file, line);
        auto pos = line.find('=');
        if (pos == std::string::npos) return "";
        return line.substr(pos + 1);
    };

    auto readInt = [&](const std::string& key) -> int {
        std::string val = readString(key);
        if (val.empty()) return 0;
        return std::stoi(val);
    };
    auto readFloat = [&](const std::string& key) -> float {
        std::string val = readString(key);
        if (val.empty()) return 0.f;
        return std::stof(val);
    };

    if (isV2) {
        data.saveTime = readString("time");
    } else {
        data.saveTime = "Unknown Time";
    }

    data.currentTurn = readInt("turn");
    data.timerEnabled = readInt("timer_enabled") != 0;
    data.timerLimit = readFloat("timer_limit");
    data.undoEnabled = readInt("undo_enabled") != 0;
    data.aiEnabled = readInt("ai_enabled") != 0;

    std::getline(file, line); // Skip "BOARD"
    
    for (int r = 0; r < BOARD_SIZE; ++r) {
        std::getline(file, line);
        std::istringstream iss(line);
        for (int c = 0; c < BOARD_SIZE; ++c) {
            std::string val;
            std::getline(iss, val, ',');
            data.grid[r][c] = std::stoi(val);
        }
    }

    std::getline(file, line); // Skip "HISTORY"
    std::getline(file, line);
    // Add safety check
    if (line.empty()) return false;
    int historySize = std::stoi(line);
    data.moveHistory.clear();
    for (int i = 0; i < historySize; ++i) {
        std::getline(file, line);
        std::istringstream iss(line);
        Move m;
        std::string val;
        std::getline(iss, val, ','); m.row = std::stoi(val);
        std::getline(iss, val, ','); m.col = std::stoi(val);
        std::getline(iss, val, ','); m.color = static_cast<Cell>(std::stoi(val));
        data.moveHistory.push_back(m);
    }

    file.close();
    std::cout << "[SaveLoad] Game loaded from: " << filename << "\n";
    return true;
}

std::string SaveLoad::exportRecord(const std::vector<Move>& history) {
    std::ostringstream ss;
    ss << "===== Gomoku Game Record =====\n";
    ss << "Total moves: " << history.size() << "\n\n";
    for (size_t i = 0; i < history.size(); ++i) {
        const auto& m = history[i];
        ss << (i + 1) << ". "
           << (m.color == Cell::BLACK ? "Black" : "White") << " "
           << Board::moveToString(m) << "\n";
    }
    ss << "\n===== End of Record =====\n";
    return ss.str();
}

