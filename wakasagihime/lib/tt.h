#ifndef TT_H
#define TT_H

#include <cstdint>
#include "types.h" // Include for Move type

// The size of the transposition table. Should be a power of 2.
// 1<<20 is about 1 million entries. Each entry is ~24 bytes. Total size: ~24 MB.
const int TABLE_SIZE = 1 << 20;

// Flags to indicate the type of score stored in a TT entry.
enum TTFlag {
    FLAG_EXACT,       // The score is an exact value (PV-node).
    FLAG_LOWER_BOUND, // The score is a lower bound (fail-high, beta-cutoff).
    FLAG_UPPER_BOUND  // The score is an upper bound (fail-low, alpha-cutoff).
};

struct TTEntry {
    uint64_t key;   // Zobrist key to verify the entry.
    int depth;      // The search depth at which this entry was stored.
    float score;    // The score of the position.
    TTFlag flag;    // The type of score (Exact, LowerBound, or UpperBound).
    Move bestMove;  // The best move found at this node.
};

// The global transposition table.
extern TTEntry tt_table[TABLE_SIZE];

// Initializes/clears the transposition table.
void tt_init();

// Probes the TT. Returns true on cutoff, always provides bestMove if available.
bool tt_probe(uint64_t key, int depth, float& alpha, float& beta, float& score, Move& bestMove);

// Stores a new entry in the transposition table.
void tt_store(uint64_t key, int depth, float score, TTFlag flag, Move bestMove);

#endif // TT_H
