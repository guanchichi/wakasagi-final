#include "lib/chess.h"
#include "lib/marisa.h"
#include "lib/types.h"
#include "lib/helper.h"
#include "lib/zobrist.h"
#include "lib/tt.h"
#include "lib/negascout.h"
#include "lib/star.h"
#include "lib/evaluate.h"
#include <vector>
#include <algorithm>
#include <limits>
#include <chrono>
#include <cmath> // For std::pow



// Evaluation functions are now in lib/evaluate.cpp

// Global variables for time management
std::chrono::time_point<std::chrono::steady_clock> stop_time;
bool time_up = false;
long long nodes_visited = 0;




// Girls are preparing...
__attribute__((constructor)) void prepare()
{
    // Prepare the distance table
    for (Square i = SQ_A1; i < SQUARE_NB; i += 1) {
        for (Square j = SQ_A1; j < SQUARE_NB; j += 1) {
            SquareDistance[i][j] = distance<Rank>(i, j) + distance<File>(i, j);
        }
    }

    // Prepare the attack table (regular)
    Direction dirs[4] = { NORTH, SOUTH, EAST, WEST };
    for (Square sq = SQ_A1; is_okay(sq); sq += 1) {
        Board a = 0;
        for (Direction d : dirs) {
            a |= safe_destination(sq, d);
        }
        PseudoAttacks[sq] = a;
    }

    // Prepare magic
    init_magic<Cannon>(cannonTable, cannonMagics);

    // Prepare Zobrist hashing
    init_zobrist();

    // Prepare Transposition Table
    tt_init();
}

long long total_time_left_ms = 600000; // 10 minutes total

// le fishe
int main()
{
    std::string line;
    /* read input board state */
    while (std::getline(std::cin, line)) {
        Position pos(line);
        MoveList moves(pos);

        if (pos.time_left() < -1.0) {
            total_time_left_ms = 600000; // Reset for new game
            continue;
        }

        auto game_start_time = std::chrono::steady_clock::now();

        uint64_t initial_key = compute_initial_key(pos);
        Color mySide = pos.due_up();

        if (mySide == Black) {
            initial_key ^= MY_SIDE_ZOBRIST_KEY;
        }

        // --- Time Management ---
        int moves_to_go;
        int max_iter_depth;
        int hidden_pieces = pos.count(Hidden);
        int total_pieces = pos.count(ALL_PIECES);

        if (hidden_pieces > 10) { // Opening
            moves_to_go = 50;
            max_iter_depth = 4;
        } else if (total_pieces > 12) { // Middlegame
            moves_to_go = 40;
            max_iter_depth = 12;
        } else { // Endgame
            moves_to_go = 20;
            max_iter_depth = 12;
        }
        
        long long move_time_ms = total_time_left_ms / moves_to_go;
        // As a safeguard, don't use more than 1/5 of the remaining time for one move
        move_time_ms = std::min(move_time_ms, total_time_left_ms / 5);
        // Hard cap of 5 seconds per move
        move_time_ms = std::min(move_time_ms, 5000LL);


        auto start_time = std::chrono::steady_clock::now();
        // Use the global stop_time
        stop_time = start_time + std::chrono::milliseconds(move_time_ms);

        int best_move_idx = -1;

        // --- Iterative Deepening ---
        for (int depth = 1; depth <= max_iter_depth; ++depth) {
            // Reset flags for each new depth search
            time_up = false;
            nodes_visited = 0;
            
            if (std::chrono::steady_clock::now() > stop_time) {
                 debug << "Time up before starting depth " << depth << "\n";
                 break;
            }

            int current_chosen_idx = -1;
            float best_score_this_iter = -std::numeric_limits<float>::infinity();

            for (int i = 0; i < moves.size(); ++i) {
                float current_score;
                Move current_move = moves[i];

                if (time_up) break; // Stop searching moves in this depth if time is up

                if (current_move.type() == Flipping) {
                    current_score = Star0_5_EQU_F(pos, initial_key, current_move, depth, -std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity(), mySide);
                } else {
                    Position next_pos = pos;
                    uint64_t next_key = initial_key;
                    Piece moving_piece = next_pos.peek_piece_at(current_move.from());
                    Piece captured_piece = next_pos.peek_piece_at(current_move.to());

                    update_key(next_key, current_move.from(), piece_to_index(moving_piece));
                    if (captured_piece.type != NO_PIECE) {
                        update_key(next_key, current_move.to(), piece_to_index(captured_piece));
                    }
                    update_key(next_key, current_move.to(), piece_to_index(moving_piece));
                    
                    next_pos.do_move(current_move);
                    current_score = G4_NegaScout(next_pos, next_key, depth - 1, -std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity(), mySide);
                }
                
                // After a move is searched, check if time ran out during the search
                if (time_up) {
                    debug << "Search for move " << current_move << " aborted due to time up.\n";
                    continue; // Don't update best score with partial results
                }

                if (current_score > best_score_this_iter) {
                    best_score_this_iter = current_score;
                    current_chosen_idx = i;
                }
            }

            // If the search for this depth was not aborted by the time flag, we can trust its result.
            if (!time_up) {
                best_move_idx = current_chosen_idx;
                if (best_move_idx != -1) {
                    debug << "Depth " << depth << " completed. Best move so far: " << moves[best_move_idx] << "\n";
                }
            } else {
                debug << "Time up during depth " << depth << ", using results from depth " << (depth - 1) << "\n";
                break; 
            }
        }
        
        auto end_time = std::chrono::steady_clock::now();
        auto time_spent_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        total_time_left_ms -= time_spent_ms;

        debug << "Time spent: " << time_spent_ms << "ms, Time left: " << total_time_left_ms << "ms\n";
        debug << "TT Stats: Hits=" << tt_hits 
              << ", Probes=" << tt_probes 
              << ", Rate=" << (tt_probes > 0 ? (double)tt_hits / tt_probes * 100.0 : 0.0) << "%\n";

        if (best_move_idx != -1) {
            /* output the move */
            info << moves[best_move_idx];
        } else {
            // Fallback: if no move was chosen (e.g. all moves lead to loss), play a random one.
            info << strategy_random(moves);
        }
        std::fflush(stderr);
    }
}

