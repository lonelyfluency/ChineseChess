#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include <string.h>
#include <limits.h>
#include "ai_agent.h"  // Include the common AI agent definitions

// Use a search depth that suits your performance needs.
#define AI_SEARCH_DEPTH 5
#define AI_BUFFER_SIZE 10

static const char* s_board =
"+ - + - + - + - + - + - + - + - + \n"
"|   |   |   | > | / |   |   |   | \n"
"+ - + - + - + - * - + - + - + - + \n"
"|   |   |   | / | > |   |   |   | \n"
"+ - # - + - + - + - + - + - # - + \n"
"|   |   |   |   |   |   |   |   | \n"
"# - + - # - + - # - + - # - + - # \n"
"|   |   |   |   |   |   |   |   | \n"
"+ - + - + - + - + - + - + - + - + \n"
"|    CHU RIVER    HAN BORDER    | \n"
"+ - + - + - + - + - + - + - + - + \n"
"|   |   |   |   |   |   |   |   | \n"
"# - + - # - + - # - + - # - + - # \n"
"|   |   |   |   |   |   |   |   | \n"
"+ - # - + - + - + - + - + - # - + \n"
"|   |   |   | > | / |   |   |   | \n"
"+ - + - + - + - * - + - + - + - + \n"
"|   |   |   | / | > |   |   |   | \n"
"+ - + - + - + - + - + - + - + - + \n\n"
"E-Up D-Down S-Left F-Right Space-Pick/Move\n"
"C-Cancel  B-Undo  R-New Game  Q-Quit\n";

static const char s_qiju_init[10][9] = {
    { 'c', 'm', 'x', 's', 'j', 's', 'x', 'm', 'c' },
    {  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0  },
    {  0 , 'p',  0 ,  0 ,  0 ,  0 ,  0 , 'p',  0  },
    { 'b',  0 , 'b',  0 , 'b',  0 , 'b',  0 , 'b' },
    {  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0  },
    {  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0  },
    { 'B',  0 , 'B',  0 , 'B',  0 , 'B',  0 , 'B' },
    {  0 , 'P',  0 ,  0 ,  0 ,  0 ,  0 , 'P',  0  },
    {  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0  },
    { 'C', 'M', 'X', 'S', 'J', 'S', 'X', 'M', 'C' },
};

static const char* get_qizi_name(char c)
{
    switch (c) {
    case 'c': return "車";
    case 'C': return "俥";
    case 'm': return "馬";
    case 'M': return "傌";
    case 'x': return "象";
    case 'X': return "相";
    case 's': return "士";
    case 'S': return "仕";
    case 'j': return "将";
    case 'J': return "帥";
    case 'p': return "砲";
    case 'P': return "炮";
    case 'b': return "卒";
    case 'B': return "兵";
    default: return "  ";
    }
}

static void limit_cursorxy(int* x, int* y)
{
    if (*x < 0) *x = 0;
    if (*x > 8) *x = 8;
    if (*y < 0) *y = 0;
    if (*y > 9) *y = 9;
}

static int check_pick(char qiju[10][9], int curx, int cury, int turn)
{
    int type = qiju[cury][curx] >= 'a' && qiju[cury][curx] <= 'z';
    return (type == turn) && qiju[cury][curx];
}

// Helper function to check if the two generals are facing each other.
static int generals_face_each_other(char board[10][9]) {
    int redGenX = -1, redGenY = -1, blackGenX = -1, blackGenY = -1;
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 9; j++) {
            if (board[i][j] == 'J') { // red general
                redGenX = j;
                redGenY = i;
            }
            if (board[i][j] == 'j') { // black general
                blackGenX = j;
                blackGenY = i;
            }
        }
    }
    if (redGenX == blackGenX && redGenX != -1) {
        int minY = (redGenY < blackGenY) ? redGenY : blackGenY;
        int maxY = (redGenY > blackGenY) ? redGenY : blackGenY;
        // Check if there is any piece between the two generals.
        for (int i = minY + 1; i < maxY; i++) {
            if (board[i][redGenX] != 0)
                return 0;
        }
        return 1;
    }
    return 0;
}

static int check_move(char qiju[10][9], int srcx, int srcy, int dstx, int dsty)
{
    int dx = abs(srcx - dstx);
    int dy = abs(srcy - dsty);
    int dist = dx + dy;
    int minx = (srcx > dstx) ? dstx : srcx;
    int maxx = (srcx > dstx) ? srcx : dstx;
    int miny = (srcy > dsty) ? dsty : srcy;
    int maxy = (srcy > dsty) ? srcy : dsty;
    int min, max, i, n;
    int srctype = (qiju[srcy][srcx] >= 'a' && qiju[srcy][srcx] <= 'z');
    int dsttype = (qiju[dsty][dstx] >= 'a' && qiju[dsty][dstx] <= 'z');
    if (qiju[dsty][dstx] && srctype == dsttype)
        return 0;

    int legal = 0;
    switch (qiju[srcy][srcx]) {
    case 'b':
        legal = (dist == 1 && dsty > srcy - (srcy >= 5));
        break;
    case 'B':
        legal = (dist == 1 && dsty < srcy + (srcy <= 4));
        break;
    case 'j':
        legal = (dstx >= 3 && dstx <= 5 && dsty <= 2 && dist == 1);
        break;
    case 'J':
        legal = (dstx >= 3 && dstx <= 5 && dsty >= 7 && dist == 1);
        break;
    case 's':
        legal = (dstx >= 3 && dstx <= 5 && dsty <= 2 && dist == 2 && dx == 1);
        break;
    case 'S':
        legal = (dstx >= 3 && dstx <= 5 && dsty >= 7 && dist == 2 && dx == 1);
        break;
    case 'x':
        legal = (dsty <= 4 && dx == 2 && dy == 2 && !qiju[miny + 1][minx + 1]);
        break;
    case 'X':
        legal = (dsty >= 5 && dx == 2 && dy == 2 && !qiju[miny + 1][minx + 1]);
        break;
    case 'm': case 'M':
        if (dist == 3) {
            if (dx == 1 && !qiju[miny + 1][srcx])
                legal = 1;
            else if (dy == 1 && !qiju[srcy][minx + 1])
                legal = 1;
            else
                legal = 0;
        }
        else
            legal = 0;
        break;
    case 'c': case 'C':
    case 'p': case 'P':
        if (srcx == dstx) {
            min = miny + 1; max = maxy - 1;
            for (n = 0, i = min; n < 2 && i <= max; i++) {
                if (qiju[i][dstx])
                    n++;
            }
        }
        else if (srcy == dsty) {
            min = minx + 1; max = maxx - 1;
            for (n = 0, i = min; n < 2 && i <= max; i++) {
                if (qiju[dsty][i])
                    n++;
            }
        }
        else {
            legal = 0;
            break;
        }
        if (n == 0)
            legal = ((qiju[srcy][srcx] == 'c' || qiju[srcy][srcx] == 'C') || !qiju[dsty][dstx]);
        else if (n == 1)
            legal = ((qiju[srcy][srcx] == 'p' || qiju[srcy][srcx] == 'P') && !!qiju[dsty][dstx]);
        else
            legal = 0;
        break;
    default:
        legal = 1;
    }

    // Additional check for the "flying general" rule.
    if (legal) {
        char captured = qiju[dsty][dstx];
        char moving = qiju[srcy][srcx];
        // Simulate the move.
        qiju[dsty][dstx] = moving;
        qiju[srcy][srcx] = 0;
        // If after the move the generals face each other,
        // then the move is legal only if it directly captures the opposing general.
        if (generals_face_each_other(qiju)) {
            if (!((moving == 'J' && captured == 'j') || (moving == 'j' && captured == 'J')))
                legal = 0;
        }
        // Undo the simulated move.
        qiju[srcy][srcx] = moving;
        qiju[dsty][dstx] = captured;
    }
    return legal;
}

static void do_move(char qiju[10][9], char srcx, char srcy, char dstx, char dsty)
{
    qiju[dsty][dstx] = qiju[srcy][srcx];
    qiju[srcy][srcx] = 0;
}

static void un_move(char qiju[10][9], char srcx, char srcy, char dstx, char dsty, char dstq)
{
    qiju[srcy][srcx] = qiju[dsty][dstx];
    qiju[dsty][dstx] = dstq;
}

static void record_move(char qipu[1024][5], int top, char srcx, char srcy, char dstx, char dsty, char dstq)
{
    if (top < 1024) {
        qipu[top][0] = srcx; qipu[top][1] = srcy;
        qipu[top][2] = dstx; qipu[top][3] = dsty;
        qipu[top][4] = dstq;
    }
}

static void draw_game(char qiju[10][9], int qiziselx, int qizisel_y, int cursorx, int cursory, int turns, char eat[2][17], char qipu[][5], int move_count)
{
    int x = 0, y = 0, i = 0, row, col, color;
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD c = { 0 };
    SetConsoleCursorPosition(h, c);
    printf("         Chinese Chess v1.0.0\n\n");
    while (s_board[i]) {
        row = y / 2; col = x / 4;
        color = (x % 4 == 0 && y % 2 == 0 && row == cursory && col == cursorx)
            ? BACKGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE
            : 0;
        if (x++ % 4 == 0 && y % 2 == 0 && qiju[row][col] && row <= 9) {
            color |= (qiju[row][col] >= 'a' && qiju[row][col] <= 'z')
                ? FOREGROUND_GREEN : FOREGROUND_RED;
            color |= (row == qizisel_y && col == qiziselx) ? 0 : FOREGROUND_INTENSITY;
            SetConsoleTextAttribute(h, color);
            printf(get_qizi_name(qiju[row][col]));
            x++; i++;
        }
        else {
            SetConsoleTextAttribute(h, color | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
            putchar(s_board[i] == '>' ? '\\' : s_board[i]);
        }
        if (s_board[i++] == '\n') { x = 0; y++; }
    }
    c.X = 35; c.Y = 3; SetConsoleCursorPosition(h, c);
    printf("%s Black", (turns & 1) ? "@" : " ");
    c.X = 35; c.Y = 11; SetConsoleCursorPosition(h, c);
    printf("Round %d", turns / 2 + 1);
    c.X = 35; c.Y = 13; SetConsoleCursorPosition(h, c);
    printf("%s Red", (turns & 1) ? " " : "@");
    for (row = 0; row < 4; row++) {
        c.X = 35; c.Y = 5 + row; SetConsoleCursorPosition(h, c);
        SetConsoleTextAttribute(h, FOREGROUND_INTENSITY | FOREGROUND_RED);
        for (col = 0; col < 4; col++)
            printf("%s", get_qizi_name(eat[1][1 + row * 4 + col]));
        c.X = 35; c.Y = 15 + row; SetConsoleCursorPosition(h, c);
        SetConsoleTextAttribute(h, FOREGROUND_INTENSITY | FOREGROUND_GREEN);
        for (col = 0; col < 4; col++)
            printf("%s", get_qizi_name(eat[0][1 + row * 4 + col]));
    }

    // Reset text attributes and move the cursor to the lower part of the screen.
    SetConsoleTextAttribute(h, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    c.X = 0; c.Y = 24; SetConsoleCursorPosition(h, c);

    // --- Display Last Move ---
    c.X = 0; c.Y = 25; SetConsoleCursorPosition(h, c);
    if (move_count > 0) {
        // Create a simulation board starting from the initial setup.
        char sim_board[10][9];
        memcpy(sim_board, s_qiju_init, sizeof(sim_board));
        // Simulate all moves except the last one.
        for (int m = 0; m < move_count - 1; m++) {
            int sx = qipu[m][0], sy = qipu[m][1];
            int dx = qipu[m][2], dy = qipu[m][3];
            do_move(sim_board, sx, sy, dx, dy);
        }
        // Retrieve the last move record.
        int srcx = qipu[move_count - 1][0];
        int srcy = qipu[move_count - 1][1];
        int dstx = qipu[move_count - 1][2];
        int dsty = qipu[move_count - 1][3];
        // Simulate the last move.
        do_move(sim_board, srcx, srcy, dstx, dsty);
        char moving_piece = sim_board[dsty][dstx];

        // Set color based on piece type.
        if (moving_piece >= 'A' && moving_piece <= 'Z') {
            // Red piece.
            SetConsoleTextAttribute(h, FOREGROUND_INTENSITY | FOREGROUND_RED);
        }
        else if (moving_piece >= 'a' && moving_piece <= 'z') {
            // Black piece.
            SetConsoleTextAttribute(h, FOREGROUND_INTENSITY | FOREGROUND_GREEN);
        }
        else {
            SetConsoleTextAttribute(h, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
        }
        // Print the moving piece.
        printf("%s", get_qizi_name(moving_piece));
        // Reset text color for the rest of the text.
        SetConsoleTextAttribute(h, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
        // Print move details.
        printf(": (%d,%d) -> (%d,%d)", srcx, srcy, dstx, dsty);
    }
    else {
        printf("No moves yet.");
    }
}


// --- Win State Check ---
// Returns 0 if no win, 1 if red wins (black general captured),
// and 2 if black wins (red general captured).
int check_win_state(char board[10][9])
{
    int redGeneralExists = 0, blackGeneralExists = 0;
    int i, j;
    for (i = 0; i < 10; i++) {
        for (j = 0; j < 9; j++) {
            if (board[i][j] == 'J')  // red general
                redGeneralExists = 1;
            if (board[i][j] == 'j')  // black general
                blackGeneralExists = 1;
        }
    }
    if (!blackGeneralExists)
        return 1; // Red wins because black general is missing.
    if (!redGeneralExists)
        return 2; // Black wins because red general is missing.
    return 0;     // No win state yet.
}


// AI define:

// --- New Data Structures for Loop Prevention ---
// A simple structure to store a move (source and destination coordinates).
typedef struct {
    int srcx, srcy, dstx, dsty;
} AIMove;

// Global move buffers for each AI agent (0 for red, 1 for black).
AIMove ai_move_buffer[2][AI_BUFFER_SIZE];
int ai_buffer_count[2] = { 0, 0 };
int ai_buffer_index[2] = { 0, 0 };

// Check whether a move is already in the AI's recent history.
int is_move_in_buffer(int turn, Move m) {
    for (int i = 0; i < ai_buffer_count[turn]; i++) {
        if (ai_move_buffer[turn][i].srcx == m.srcx &&
            ai_move_buffer[turn][i].srcy == m.srcy &&
            ai_move_buffer[turn][i].dstx == m.dstx &&
            ai_move_buffer[turn][i].dsty == m.dsty) {
            return 1;  // Found in buffer.
        }
    }
    return 0;
}

// Assign values to pieces (evaluation is from red’s perspective).
int piece_value(char piece) {
    switch (piece) {
    case 'J': return 10000;  // red general
    case 'j': return 10000;  // black general (will be subtracted later)
    case 'C': case 'c': return 90;   // chariot/rook
    case 'M': case 'm': return 45;   // horse
    case 'X': case 'x': return 20;   // elephant
    case 'S': case 's': return 20;   // advisor
    case 'P': case 'p': return 45;   // cannon
    case 'B': case 'b': return 10;   // soldier
    default:    return 0;
    }
}

// Evaluation function: sum the values of red pieces and subtract black’s.
int evaluate_board(char board[10][9]) {
    int score = 0;
    int i, j;
    int redGeneralFound = 0, blackGeneralFound = 0;
    for (i = 0; i < 10; i++) {
        for (j = 0; j < 9; j++) {
            char p = board[i][j];
            if (p) {
                if (p >= 'A' && p <= 'Z') { // red piece
                    score += piece_value(p);
                    if (p == 'J') redGeneralFound = 1;
                }
                else if (p >= 'a' && p <= 'z') { // black piece
                    score -= piece_value(p);
                    if (p == 'j') blackGeneralFound = 1;
                }
            }
        }
    }
    // Terminal conditions: if a general is missing, return a decisive score.
    if (!redGeneralFound) return -10000;
    if (!blackGeneralFound) return  10000;
    return score;
}

// Generate all legal moves for the current side.
// turn: 0 for red (uppercase), 1 for black (lowercase).
// moves: an array to fill; max_moves is its capacity.
int generate_moves(char board[10][9], int turn, Move moves[], int max_moves) {
    int count = 0;
    int x, y, dstx, dsty;
    // Loop over all board positions.
    for (y = 0; y < 10; y++) {
        for (x = 0; x < 9; x++) {
            if (board[y][x]) {
                // Determine which side the piece belongs to.
                int pieceTurn = (board[y][x] >= 'a' && board[y][x] <= 'z') ? 1 : 0;
                if (pieceTurn == turn) {
                    // Try every destination square.
                    for (dsty = 0; dsty < 10; dsty++) {
                        for (dstx = 0; dstx < 9; dstx++) {
                            if (dstx == x && dsty == y)
                                continue;
                            // Use your existing check_move() to validate the move.
                            if (check_move(board, x, y, dstx, dsty)) {
                                if (count < max_moves) {
                                    moves[count].srcx = x;
                                    moves[count].srcy = y;
                                    moves[count].dstx = dstx;
                                    moves[count].dsty = dsty;
                                    moves[count].captured = board[dsty][dstx];
                                    count++;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return count;
}

// Minimax search with alpha-beta pruning.
// The evaluation function is from red’s perspective.
// When turn == 0 (red), we maximize; when turn == 1 (black), we minimize.
int minimax(char board[10][9], int depth, int turn, int alpha, int beta) {
    if (depth == 0)
        return evaluate_board(board);

    Move moves[500];
    int moveCount = generate_moves(board, turn, moves, 500);
    if (moveCount == 0)
        return evaluate_board(board);

    int i, score;
    if (turn == 0) {  // Red’s turn: maximize score.
        int maxEval = -INT_MAX;
        for (i = 0; i < moveCount; i++) {
            Move m = moves[i];
            do_move(board, m.srcx, m.srcy, m.dstx, m.dsty);
            score = minimax(board, depth - 1, 1, alpha, beta);
            un_move(board, m.srcx, m.srcy, m.dstx, m.dsty, m.captured);
            if (score > maxEval)
                maxEval = score;
            if (maxEval > alpha)
                alpha = maxEval;
            if (beta <= alpha)
                break;  // Beta cutoff.
        }
        return maxEval;
    }
    else {  // Black’s turn: minimize score.
        int minEval = INT_MAX;
        for (i = 0; i < moveCount; i++) {
            Move m = moves[i];
            do_move(board, m.srcx, m.srcy, m.dstx, m.dsty);
            score = minimax(board, depth - 1, 0, alpha, beta);
            un_move(board, m.srcx, m.srcy, m.dstx, m.dsty, m.captured);
            if (score < minEval)
                minEval = score;
            if (minEval < beta)
                beta = minEval;
            if (beta <= alpha)
                break;  // Alpha cutoff.
        }
        return minEval;
    }
}

// For each candidate, we compute the minimax score. We track the best candidate overall
// and also the best candidate that is not in the AI’s move buffer.
Move get_best_move(char board[10][9], int turn, int depth) {
    Move best_move = { 0, 0, 0, 0, 0 }, best_move_nonloop = { 0, 0, 0, 0, 0 };
    int best_eval, best_eval_nonloop, score;
    int found_nonloop = 0;

    if (turn == 0) { // Red (maximize)
        best_eval = -INT_MAX;
        best_eval_nonloop = -INT_MAX;
    }
    else {        // Black (minimize)
        best_eval = INT_MAX;
        best_eval_nonloop = INT_MAX;
    }

    Move moves[500];
    int moveCount = generate_moves(board, turn, moves, 500);
    if (moveCount == 0)
        return best_move; // No legal moves.

    for (int i = 0; i < moveCount; i++) {
        Move m = moves[i];
        do_move(board, m.srcx, m.srcy, m.dstx, m.dsty);
        // Use minimax for the opponent (1-turn) with depth-1.
        score = minimax(board, depth - 1, 1 - turn, -INT_MAX, INT_MAX);
        un_move(board, m.srcx, m.srcy, m.dstx, m.dsty, m.captured);

        if (turn == 0) { // Red: maximize score.
            if (score > best_eval) {
                best_eval = score;
                best_move = m;
            }
            if (!is_move_in_buffer(turn, m) && score > best_eval_nonloop) {
                best_eval_nonloop = score;
                best_move_nonloop = m;
                found_nonloop = 1;
            }
        }
        else {         // Black: minimize score.
            if (score < best_eval) {
                best_eval = score;
                best_move = m;
            }
            if (!is_move_in_buffer(turn, m) && score < best_eval_nonloop) {
                best_eval_nonloop = score;
                best_move_nonloop = m;
                found_nonloop = 1;
            }
        }
    }
    // Prefer a candidate that is not in the move buffer.
    if (found_nonloop)
        return best_move_nonloop;
    return best_move;
}

// Wrapper function for the minimax agent.
Move minimax_agent(char board[10][9], int turn) {
    return get_best_move(board, turn, AI_SEARCH_DEPTH);
}

int main(void)
{
    char qiju[10][9], eat[2][17] = { 0 }, qipu[1024][5];
    int cursorx[2] = { 4, 4 }, cursory[2] = { 9, 0 };
    int qizisel_x = -1, qizisel_y = -1;
    int turn = 0, numt = 0, key = 0;

    memcpy(qiju, s_qiju_init, sizeof(qiju));

    // Let the user choose the game mode.
    // ai_control[0] controls red; ai_control[1] controls black.
    int ai_control[2] = { 0, 0 };
    printf("Select game mode:\n");
    printf("1. Player vs Player\n");
    printf("2. Player (red) vs AI (black)\n");
    printf("3. AI (red) vs Player (black)\n");
    printf("4. AI vs AI\n");
    key = _getch();
    switch (key) {
    case '2': ai_control[0] = 0; ai_control[1] = 1; break;
    case '3': ai_control[0] = 1; ai_control[1] = 0; break;
    case '4': ai_control[0] = 1; ai_control[1] = 1; break;
    default:  ai_control[0] = 0; ai_control[1] = 0; break;
    }

    // --- AI Agent Assignment ---
    // For demonstration, we assign both sides to use the minimax agent.
    extern Move random_agent(char board[10][9], int turn); // From random_agent.c
    //AIAgent ai_agent[2] = { minimax_agent, random_agent };
    AIAgent ai_agent[2] = { minimax_agent, minimax_agent };

    // Main game loop.
    while (key != 'q' && key != 'Q') {
        turn = numt & 1;
        // Pass move record and count so draw_game can display the last move.
        draw_game(qiju, qizisel_x, qizisel_y, cursorx[turn], cursory[turn], numt, eat, qipu, numt);

        // Check for a win state at the beginning of each iteration.
        int win_state = check_win_state(qiju);
        if (win_state != 0) {
            draw_game(qiju, qizisel_x, qizisel_y, cursorx[turn], cursory[turn], numt, eat, qipu, numt);
            if (win_state == 1)
                printf("Red wins!\n");
            else if (win_state == 2)
                printf("Black wins!\n");
            printf("Press any key to exit or R to restart...\n");
            key = _getch();
            if (key == 'r' || key == 'R') {
                memcpy(qiju, s_qiju_init, sizeof(qiju));
                memset(eat, 0, sizeof(eat));
                cursorx[0] = 4; cursorx[1] = 4;
                cursory[0] = 9; cursory[1] = 0;
                numt = 0; qizisel_x = -1; qizisel_y = -1;
                continue;
            }
            else {
                break;
            }
        }

        // --- AI Move Section ---
        if (ai_control[turn]) {
            printf("AI is thinking...\n");
            Move ai_move = ai_agent[turn](qiju, turn);
            if (check_move(qiju, ai_move.srcx, ai_move.srcy, ai_move.dstx, ai_move.dsty)) {
                if (qiju[ai_move.dsty][ai_move.dstx])
                    eat[turn][++eat[turn][0]] = qiju[ai_move.dsty][ai_move.dstx];
                record_move(qipu, numt, ai_move.srcx, ai_move.srcy, ai_move.dstx, ai_move.dsty,
                    qiju[ai_move.dsty][ai_move.dstx]);
                do_move(qiju, ai_move.srcx, ai_move.srcy, ai_move.dstx, ai_move.dsty);
                cursorx[turn] = ai_move.dstx;
                cursory[turn] = ai_move.dsty;
                numt++;
                qizisel_x = -1;
                qizisel_y = -1;

                AIMove new_move;
                new_move.srcx = ai_move.srcx;
                new_move.srcy = ai_move.srcy;
                new_move.dstx = ai_move.dstx;
                new_move.dsty = ai_move.dsty;
                ai_move_buffer[turn][ai_buffer_index[turn]] = new_move;
                ai_buffer_index[turn] = (ai_buffer_index[turn] + 1) % AI_BUFFER_SIZE;
                if (ai_buffer_count[turn] < AI_BUFFER_SIZE)
                    ai_buffer_count[turn]++;
            }
            Sleep(500);
            continue;
        }

        // Process user input.
        switch ((key = _getch())) {
        case 'e': case 'E': cursory[turn]--; break; // up
        case 'd': case 'D': cursory[turn]++; break; // down
        case 's': case 'S': cursorx[turn]--; break; // left
        case 'f': case 'F': cursorx[turn]++; break; // right
        case 'c': case 'C': qizisel_x = qizisel_y = -1; break;
        case 'r': case 'R':
            printf("Restart game? (Y/N) ");
            key = _getch();
            if (key == 'y' || key == 'Y') {
                memcpy(qiju, s_qiju_init, sizeof(qiju));
                memset(eat, 0, sizeof(eat));
                cursorx[0] = 4; cursorx[1] = 4;
                cursory[0] = 9; cursory[1] = 0;
                numt = 0; qizisel_x = -1; qizisel_y = -1;
            }
            else {
                system("cls");
            }
            break;
        case ' ':
            if (qizisel_x == -1 && check_pick(qiju, cursorx[turn], cursory[turn], turn)) {
                qizisel_x = cursorx[turn];
                qizisel_y = cursory[turn];
            }
            else if (qizisel_x != -1 && check_move(qiju, qizisel_x, qizisel_y, cursorx[turn], cursory[turn])) {
                if (qiju[cursory[turn]][cursorx[turn]])
                    eat[turn][++eat[turn][0]] = qiju[cursory[turn]][cursorx[turn]];
                record_move(qipu, numt, qizisel_x, qizisel_y, cursorx[turn], cursory[turn],
                    qiju[cursory[turn]][cursorx[turn]]);
                do_move(qiju, qizisel_x, qizisel_y, cursorx[turn], cursory[turn]);
                qizisel_x = -1; qizisel_y = -1;
                numt++;
            }
            break;
        case 'b': case 'B':
            if (numt > 0) {
                numt--;
                un_move(qiju, qipu[numt][0], qipu[numt][1], qipu[numt][2], qipu[numt][3], qipu[numt][4]);
                cursorx[numt & 1] = qipu[numt][0];
                cursory[numt & 1] = qipu[numt][1];
                if (qipu[numt][4])
                    eat[numt & 1][eat[numt & 1][0]--] = 0;
            }
            if (numt > 0) {
                numt--;
                un_move(qiju, qipu[numt][0], qipu[numt][1], qipu[numt][2], qipu[numt][3], qipu[numt][4]);
                cursorx[numt & 1] = qipu[numt][0];
                cursory[numt & 1] = qipu[numt][1];
                if (qipu[numt][4])
                    eat[numt & 1][eat[numt & 1][0]--] = 0;
            }
            qizisel_x = -1; qizisel_y = -1;
            break;
        }
        limit_cursorxy(cursorx + turn, cursory + turn);
    }
    return 0;
}
