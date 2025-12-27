#ifndef STAR_H
#define STAR_H

#include "chess.h"
#include "types.h"

float Star1_EQU_F(Position& pos, uint64_t key, Move flip_move, int depth, float alpha, float beta, Color mySide);
float Star1_EQU_G(Position& pos, uint64_t key, Move flip_move, int depth, float alpha, float beta, Color mySide);

#endif // STAR_H
