#pragma execution_character_set("utf-8")
#include "Game.h"
#include <iostream>

// ============================================================
//  main.cpp Program Entry Point
// ============================================================

int main() {
    try {
        std::cout << "Starting Gomoku v3 (SFML 3.0.2)...\n";
        Game game;
        game.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal Exception: " << e.what() << "\n";
        return -1;
    } catch (...) {
        std::cerr << "Unknown Fatal Exception!\n";
        return -1;
    }
    return 0;
}
