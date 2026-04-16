#pragma execution_character_set("utf-8")
#pragma once
#include "Board.h"
#include <vector>
#include <limits>
#include <atomic>





class AI {
public:
    AI(Cell aiColor);

    void setColor(Cell aiColor);
    Cell getColor() const { return m_aiColor; }
    void setDepth(int depth);
    Move calculateBestMove(const Board& board);

private:
    Cell m_aiColor;
    Cell m_opponentColor;
    int m_maxDepth = 2;




    int evaluateBoard(const Board& board, Cell color) const;


    int evaluateLine(int count, int openEnds) const;
    std::vector<Move> getCandidateMoves(const Board& board) const;
    int minimax(Board& board, int depth, int alpha, int beta, bool isMaximizing);

    std::atomic<int> m_globalAlpha{std::numeric_limits<int>::min()};
};

