#pragma execution_character_set("utf-8")
#include "AI.h"
#include <algorithm>
#include <future>
#include <vector>


// wc ai


AI::AI(Cell aiColor) {
	setColor(aiColor);
}

void AI::setColor(Cell aiColor) {
	m_aiColor = aiColor;
	m_opponentColor = (aiColor == Cell::BLACK) ? Cell::WHITE : Cell::BLACK;
}

void AI::setDepth(int depth) {
	m_maxDepth = depth;
}

Move AI::calculateBestMove(const Board& board) {

	if (board.getMoveCount() == 0) {
		return { BOARD_SIZE / 2, BOARD_SIZE / 2, m_aiColor };
	}

	Board tempBoard;
	auto history = board.getMoveHistory();
	for (const auto& m : history) {
		tempBoard.placeStone(m.row, m.col, m.color);
	}

	std::vector<Move> candidates = getCandidateMoves(tempBoard);
	if (candidates.empty()) return { -1, -1, Cell::EMPTY };

	int bestScore = std::numeric_limits<int>::min();
	Move bestMove = candidates[0];

	std::vector<std::future<std::pair<Move, int>>> futures;

	m_globalAlpha.store(std::numeric_limits<int>::min());

	for (const auto& move : candidates) {
		// 使用 std::async 平行計算每個候選步的分數 (複製 tempBoard，確保不會有 thread 衝突)
		futures.push_back(std::async(std::launch::async, [this, tempBoard, move]() mutable {
			tempBoard.placeStone(move.row, move.col, m_aiColor);

			int score = minimax(tempBoard, m_maxDepth - 1, m_globalAlpha.load(), std::numeric_limits<int>::max(), false);

			int current = m_globalAlpha.load();
			while (score > current && !m_globalAlpha.compare_exchange_weak(current, score)) {
			}

			return std::make_pair(move, score);
			}));
	}

	for (auto& f : futures) {
		auto result = f.get();
		if (result.second > bestScore) {
			bestScore = result.second;
			bestMove = result.first;
		}
	}

	bestMove.color = m_aiColor;
	return bestMove;
}

int AI::minimax(Board& board, int depth, int alpha, int beta, bool isMaximizing) {
	Cell winner = board.checkWin();
	if (winner == m_aiColor) return 1000000 + depth;
	if (winner == m_opponentColor) return -1000000 - depth;
	if (board.isFull() || depth == 0) {
		return evaluateBoard(board, m_aiColor) - evaluateBoard(board, m_opponentColor);
	}

	std::vector<Move> candidates = getCandidateMoves(board);

	if (isMaximizing) {
		int maxEval = std::numeric_limits<int>::min();
		for (const auto& move : candidates) {
			int currentGlobalAlpha = m_globalAlpha.load();
			if (currentGlobalAlpha > alpha) alpha = currentGlobalAlpha;

			board.placeStone(move.row, move.col, m_aiColor);
			int eval = minimax(board, depth - 1, alpha, beta, false);
			board.undoMove();

			maxEval = std::max(maxEval, eval);
			alpha = std::max(alpha, eval);
			if (beta <= alpha) break;
		}
		return maxEval;
	}
	else {
		int minEval = std::numeric_limits<int>::max();
		for (const auto& move : candidates) {
			int currentGlobalAlpha = m_globalAlpha.load();
			if (beta <= currentGlobalAlpha) break;

			board.placeStone(move.row, move.col, m_opponentColor);
			int eval = minimax(board, depth - 1, std::max(alpha, currentGlobalAlpha), beta, true);
			board.undoMove();

			minEval = std::min(minEval, eval);
			beta = std::min(beta, eval);
			if (beta <= std::max(alpha, currentGlobalAlpha)) break;
		}
		return minEval;
	}
}

int AI::evaluateBoard(const Board& board, Cell color) const {
	int totalScore = 0;

	const int dirs[4][2] = { {1, 0}, {0, 1}, {1, 1}, {1, -1} };

	for (int r = 0; r < BOARD_SIZE; ++r) {
		for (int c = 0; c < BOARD_SIZE; ++c) {
			if (board.getCell(r, c) != color) continue;

			for (auto& d : dirs) {
				int dr = d[0], dc = d[1];

				if (r - dr >= 0 && r - dr < BOARD_SIZE && c - dc >= 0 && c - dc < BOARD_SIZE && board.getCell(r - dr, c - dc) == color) {
					continue;
				}

				int count = 0;
				int openEnds = 0;


				if (r - dr >= 0 && r - dr < BOARD_SIZE && c - dc >= 0 && c - dc < BOARD_SIZE && board.getCell(r - dr, c - dc) == Cell::EMPTY) {
					openEnds++;
				}


				int currR = r, currC = c;
				while (currR >= 0 && currR < BOARD_SIZE && currC >= 0 && currC < BOARD_SIZE && board.getCell(currR, currC) == color) {
					count++;
					currR += dr;
					currC += dc;
				}


				if (currR >= 0 && currR < BOARD_SIZE && currC >= 0 && currC < BOARD_SIZE && board.getCell(currR, currC) == Cell::EMPTY) {
					openEnds++;
				}

				totalScore += evaluateLine(count, openEnds);
			}
		}
	}

	return totalScore;
}

int AI::evaluateLine(int count, int openEnds) const {
	if (count >= 5) return 100000;
	if (count == 4 && openEnds == 2) return 10000;
	if (count == 4 && openEnds == 1) return 1000;
	if (count == 3 && openEnds == 2) return 1000;
	if (count == 3 && openEnds == 1) return 100;
	if (count == 2 && openEnds == 2) return 100;
	if (count == 2 && openEnds == 1) return 10;
	return 0;
}

std::vector<Move> AI::getCandidateMoves(const Board& board) const {
	std::vector<Move> moves;
	moves.reserve(BOARD_SIZE * BOARD_SIZE);
	bool hasNeighbor[BOARD_SIZE][BOARD_SIZE] = { false };


	for (int r = 0; r < BOARD_SIZE; ++r) {
		for (int c = 0; c < BOARD_SIZE; ++c) {
			if (board.getCell(r, c) != Cell::EMPTY) {
				for (int dr = -2; dr <= 2; ++dr) {
					for (int dc = -2; dc <= 2; ++dc) {
						int nr = r + dr;
						int nc = c + dc;
						if (nr >= 0 && nr < BOARD_SIZE && nc >= 0 && nc < BOARD_SIZE) {
							if (board.getCell(nr, nc) == Cell::EMPTY && !hasNeighbor[nr][nc]) {
								hasNeighbor[nr][nc] = true;
								Move m;
								m.row = nr;
								m.col = nc;
								moves.push_back(m);
							}
						}
					}
				}
			}
		}
	}



	auto countNeighbors = [](const Board& bd, int r, int c) {
		int count = 0;
		for (int dr = -2; dr <= 2; ++dr) {
			for (int dc = -2; dc <= 2; ++dc) {
				int nr = r + dr, nc = c + dc;
				if (nr >= 0 && nr < BOARD_SIZE && nc >= 0 && nc < BOARD_SIZE) {
					if (bd.getCell(nr, nc) != Cell::EMPTY) count++;
				}
			}
		}
		return count;
		};

	std::sort(moves.begin(), moves.end(), [&board, &countNeighbors](const Move& a, const Move& b) {
		return countNeighbors(board, a.row, a.col) > countNeighbors(board, b.row, b.col);
		});

	return moves;
}

