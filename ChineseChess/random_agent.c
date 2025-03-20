#include <stdlib.h>
#include "ai_agent.h"

// Forward declaration of generate_moves from the engine.
extern int generate_moves(char board[10][9], int turn, Move moves[], int max_moves);

// A random AI agent: It generates all legal moves and returns one at random.
Move random_agent(char board[10][9], int turn) {
    Move moves[500];
    int moveCount = generate_moves(board, turn, moves, 500);
    if (moveCount == 0) {
        Move m = { 0, 0, 0, 0, 0 };
        return m;
    }
    int index = rand() % moveCount;
    return moves[index];
}
