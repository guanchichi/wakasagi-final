#include "zobrist.h"
#include <random>

uint64_t zobrist_table[32][15];

int piece_to_index(const Piece& p) {
    if (p.type == Hidden) {
        return 14;
    }
    // Check for valid piece types that can be on the board
    if (p.type >= General && p.type <= Soldier) {
        if (p.side == Red) {
            return p.type;
        }
        if (p.side == Black) {
            return 7 + p.type;
        }
    }
    // Return -1 for empty squares or other piece types (like Duck) not included in hashing
    return -1;
}

void init_zobrist() {
    // Using a fixed seed ensures that the Zobrist keys are deterministic,
    // which is essential for debugging and for transposition tables to work correctly
    // across different runs of the program.
    std::mt19937_64 rng(20240521); 
    for (int i = 0; i < 32; ++i) {
        for (int j = 0; j < 15; ++j) {
            zobrist_table[i][j] = rng();
        }
    }
}

uint64_t compute_initial_key(const Position& pos) {
    uint64_t key = 0;
    for (Square sq = SQ_A1; sq < SQUARE_NB; sq += 1) {
        Piece p = pos.peek_piece_at(sq);
        if (p.type != NO_PIECE) {
            int piece_idx = piece_to_index(p);
            if (piece_idx != -1) {
                key ^= zobrist_table[sq][piece_idx];
            }
        }
    }
    return key;
}

void update_key(uint64_t& current_key, Square sq, int piece_idx) {
    if (piece_idx != -1 && is_okay(sq)) {
        current_key ^= zobrist_table[sq][piece_idx];
    }
}
