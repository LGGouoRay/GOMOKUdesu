#pragma execution_character_set("utf-8")
#include "Board.h"
#include <algorithm>
#include <sstream>





Board::Board() {
    reset();
}

void Board::reset() {
    for (auto& row : m_grid)
        row.fill(Cell::EMPTY);
    m_moveHistory.clear();
}

bool Board::placeStone(int row, int col, Cell color) {
    if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE)
        return false;
    if (m_grid[row][col] != Cell::EMPTY)
        return false;
    if (color == Cell::EMPTY)
        return false;

    m_grid[row][col] = color;
    m_moveHistory.push_back({ row, col, color });
    return true;
}

bool Board::undoMove() {
    if (m_moveHistory.empty())
        return false;

    const Move& last = m_moveHistory.back();
    m_grid[last.row][last.col] = Cell::EMPTY;
    m_moveHistory.pop_back();
    return true;
}

bool Board::removeStone(int row, int col) {
    if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE)
        return false;
    if (m_grid[row][col] == Cell::EMPTY)
        return false;

    m_grid[row][col] = Cell::EMPTY;
    // Remove from history if it exists
    auto it = std::find_if(m_moveHistory.begin(), m_moveHistory.end(),
                           [row, col](const Move& m) { return m.row == row && m.col == col; });
    if (it != m_moveHistory.end()) {
        m_moveHistory.erase(it);
    }
    return true;
}

Cell Board::checkWin() const {
    int dummyRow[5], dummyCol[5];
    for (int r = 0; r < BOARD_SIZE; ++r) {
        for (int c = 0; c < BOARD_SIZE; ++c) {
            if (m_grid[r][c] == Cell::EMPTY) continue;
            Cell color = m_grid[r][c];

            if (checkDirection(r, c, 0, 1, color, dummyRow, dummyCol)) return color;
            if (checkDirection(r, c, 1, 0, color, dummyRow, dummyCol)) return color;
            if (checkDirection(r, c, 1, 1, color, dummyRow, dummyCol)) return color;
            if (checkDirection(r, c, 1, -1, color, dummyRow, dummyCol)) return color;
        }
    }
    return Cell::EMPTY;
}

bool Board::getWinLine(int outRow[5], int outCol[5]) const {
    for (int r = 0; r < BOARD_SIZE; ++r) {
        for (int c = 0; c < BOARD_SIZE; ++c) {
            if (m_grid[r][c] == Cell::EMPTY) continue;
            Cell color = m_grid[r][c];
            if (checkDirection(r, c, 0, 1, color, outRow, outCol)) return true;
            if (checkDirection(r, c, 1, 0, color, outRow, outCol)) return true;
            if (checkDirection(r, c, 1, 1, color, outRow, outCol)) return true;
            if (checkDirection(r, c, 1, -1, color, outRow, outCol)) return true;
        }
    }
    return false;
}

bool Board::isFull() const {
    for (auto& row : m_grid)
        for (auto& cell : row)
            if (cell == Cell::EMPTY) return false;
    return true;
}

Cell Board::getCell(int row, int col) const {
    if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE)
        return Cell::EMPTY;
    return m_grid[row][col];
}

const std::vector<Move>& Board::getMoveHistory() const {
    return m_moveHistory;
}

int Board::getMoveCount() const {
    return static_cast<int>(m_moveHistory.size());
}

const std::array<std::array<Cell, BOARD_SIZE>, BOARD_SIZE>& Board::getGrid() const {
    return m_grid;
}

void Board::setGrid(const std::array<std::array<Cell, BOARD_SIZE>, BOARD_SIZE>& grid) {
    m_grid = grid;
}

void Board::setMoveHistory(const std::vector<Move>& history) {
    m_moveHistory = history;
}

bool Board::changeStoneColor(int row, int col, Cell newColor) {
    if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE) return false;
    if (m_grid[row][col] == Cell::EMPTY) return false;
    m_grid[row][col] = newColor;

    // Update history
    auto it = std::find_if(m_moveHistory.begin(), m_moveHistory.end(),
                           [row, col](const Move& m) { return m.row == row && m.col == col; });
    if (it != m_moveHistory.end()) {
        it->color = newColor;
    }
    return true;
}

std::string Board::moveToString(const Move& m) {
    if (m.row < 0 || m.col < 0) return "??";
    std::ostringstream ss;
    ss << static_cast<char>('A' + m.col) << (BOARD_SIZE - m.row);
    return ss.str();
}

bool Board::checkDirection(int r, int c, int dr, int dc, Cell color,
    int outRow[5], int outCol[5]) const {
    for (int i = 0; i < 5; ++i) {
        int nr = r + i * dr;
        int nc = c + i * dc;
        if (nr < 0 || nr >= BOARD_SIZE || nc < 0 || nc >= BOARD_SIZE)
            return false;
        if (m_grid[nr][nc] != color)
            return false;
        outRow[i] = nr;
        outCol[i] = nc;
    }
    return true;
}

