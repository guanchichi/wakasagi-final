#ifndef NEGASCOUT_H
#define NEGASCOUT_H

#include "chess.h"
#include "types.h"

float F4_NegaScout(Position& pos, uint64_t key, int depth, float alpha, float beta, Color mySide);
float G4_NegaScout(Position& pos, uint64_t key, int depth, float alpha, float beta, Color mySide);

#endif // NEGASCOUT_H
