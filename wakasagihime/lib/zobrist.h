#ifndef ZOBRIST_H
#define ZOBRIST_H

#include <cstdint>
#include "chess.h" // For Position and Piece

// Zobrist table: 32 squares, 15 piece states
// Piece states: Red(7), Black(7), Hidden(1)
extern uint64_t zobrist_table[32][15];

// Helper to map a Piece object to its corresponding index in the Zobrist table
int piece_to_index(const Piece& p);

// Initializes the Zobrist table with random numbers
void init_zobrist();

// Computes the initial Zobrist key for a position from scratch
uint64_t compute_initial_key(const Position& pos);

// Incrementally updates a key by XORing a piece state at a given square
void update_key(uint64_t& current_key, Square sq, int piece_idx);

#endif // ZOBRIST_H
