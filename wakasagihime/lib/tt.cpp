#include "tt.h"
#include <cstring> // For std::memset
#include <algorithm>

long long tt_probes = 0;
long long tt_hits = 0;
TTEntry tt_table[TABLE_SIZE];

void tt_init() {
    // Clear the entire transposition table.
    // This sets all entries to a default (zero) state.
    std::memset(tt_table, 0, sizeof(tt_table));
}

bool tt_probe(uint64_t key, int depth, float& alpha, float& beta, float& score, Move& bestMove) {
    tt_probes++;
    TTEntry& entry = tt_table[key & (TABLE_SIZE - 1)];

    // Key mismatch or entry too shallow → unusable
    if (entry.key != key || entry.depth < depth)
        return false;

    tt_hits++;
    
    // Provide best move for move ordering
    bestMove = entry.bestMove;

    // Exact score → immediate return
    if (entry.flag == FLAG_EXACT) {
        score = entry.score;
        return true;
    }

    // Bound handling (VERY important for Negascout)
    if (entry.flag == FLAG_LOWER_BOUND) {
        alpha = std::max(alpha, entry.score);
    } else if (entry.flag == FLAG_UPPER_BOUND) {
        beta = std::min(beta, entry.score);
    }

    // Check if bounds cause a cutoff
    if (alpha >= beta) {
        score = entry.score;
        return true;
    }

    return false;
}

void tt_store(uint64_t key, int depth, float score, TTFlag flag, Move bestMove) {
    TTEntry& entry = tt_table[key & (TABLE_SIZE - 1)];

    // Depth-preferred replacement policy
    if (entry.key != key || depth >= entry.depth) {
        entry.key      = key;
        entry.depth    = depth;
        entry.score    = score;
        entry.flag     = flag;
        entry.bestMove = bestMove;
    }
}