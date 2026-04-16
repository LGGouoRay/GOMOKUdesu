#pragma execution_character_set("utf-8")
#pragma once
#include <vector>
#include <string>
#include <array>




constexpr int BOARD_SIZE = 15;

enum class Cell : int { EMPTY = 0, BLACK = 1, WHITE = 2 };

struct Move {
    int row = -1;
    int col = -1;
    Cell color = Cell::EMPTY;
};

class Board {
public:
    Board();


    void reset();


    bool placeStone(int row, int col, Cell color);


    bool undoMove();
    bool removeStone(int row, int col);


    Cell checkWin() const;


    bool getWinLine(int outRow[5], int outCol[5]) const;


    bool isFull() const;

    Cell getCell(int row, int col) const;

    const std::vector<Move>& getMoveHistory() const;


    int getMoveCount() const;


    const std::array<std::array<Cell, BOARD_SIZE>, BOARD_SIZE>& getGrid() const;


    void setGrid(const std::array<std::array<Cell, BOARD_SIZE>, BOARD_SIZE>& grid);
    void setMoveHistory(const std::vector<Move>& history);
    bool changeStoneColor(int row, int col, Cell newColor);


    static std::string moveToString(const Move& m);

private:
    std::array<std::array<Cell, BOARD_SIZE>, BOARD_SIZE> m_grid;
    std::vector<Move> m_moveHistory;


    bool checkDirection(int r, int c, int dr, int dc, Cell color, int outRow[5], int outCol[5]) const;
};

