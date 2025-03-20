#ifndef AI_AGENT_H
#define AI_AGENT_H

// Structure to represent a move.
typedef struct {
    int srcx, srcy, dstx, dsty;
    char captured;  // Captured piece (if any) for undo purposes.
} Move;

// Function pointer type for an AI agent.
// Given the current board and which turn (0 for red, 1 for black),
// the agent returns a Move.
typedef Move(*AIAgent)(char board[10][9], int turn);

#endif // AI_AGENT_H
