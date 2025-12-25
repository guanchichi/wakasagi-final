#ifndef EVALUATE_H
#define EVALUATE_H

#include "chess.h"
#include "types.h"

float evaluate(const Position& pos, Color mySide);
float get_flip_score(const Position& pos, Square sq, Color mySide);
int get_move_score(const Move& m, const Position& pos, Color mySide);
int piece_value(PieceType pt);

// Flipping Heuristics (Table II from paper)
const std::array<int, MOVABLE_PIECE_TYPE_NB> FlippingHeuristicOpponent = {
    -5000, // General (King)
    -800,  // Advisor (Guard)
    -600,  // Elephant (Minister)
    -400,  // Chariot (Rook)
    -200,  // Horse (Knight)
    +200,  // Cannon
    -5000  // Soldier (Pawn)
};

const std::array<int, MOVABLE_PIECE_TYPE_NB> FlippingHeuristicMySide = {
    -5000, // General (King)
    +800,  // Advisor (Guard)
    +600,  // Elephant (Minister)
    +200,  // Chariot (Rook)
    +100,  // Horse (Knight)
    -5000, // Cannon
    -500   // Soldier (Pawn)
};

// Influence Values (Table IV from paper)
// [Attacker][Defender] matrix
const int InfluenceValues[MOVABLE_PIECE_TYPE_NB][MOVABLE_PIECE_TYPE_NB] = {
    // Defender: General, Advisor, Elephant, Chariot, Horse, Cannon, Soldier
    {120, 108, 60, 36, 24, 48, 0},   // Attacker: General
    {0, 106, 60, 36, 24, 48, 12},    // Attacker: Advisor
    {0, 0, 58, 36, 24, 48, 12},      // Attacker: Elephant
    {0, 0, 0, 34, 24, 48, 12},       // Attacker: Chariot
    {0, 0, 0, 0, 22, 48, 12},        // Attacker: Horse
    {0, 0, 0, 0, 0, 0, 0},           // Attacker: Cannon
    {108, 0, 0, 0, 0, 0, 10}         // Attacker: Soldier
};
#endif // EVALUATE_H
