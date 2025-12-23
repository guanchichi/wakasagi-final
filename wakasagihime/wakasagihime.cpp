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



// Evaluation function
int piece_value(PieceType pt) {
    switch (pt) {
        case General:  return 100;
        case Advisor:  return 40;
        case Elephant: return 20;
        case Chariot:  return 10;
        case Horse:    return 5;
        case Cannon:   return 15;
        case Soldier:  return 2;
        default:       return 0;
    }
}

float evaluate(const Position& pos, Color mySide) {
    float score = 0;
    Color us = mySide;
    Color them = ~us;

    for (PieceType pt = General; pt < SHOWN_PIECE_TYPE_NB; pt += 1) {
        score += pos.count(us, pt) * piece_value(pt);
        score -= pos.count(them, pt) * piece_value(pt);
    }

    // From the perspective of the player whose turn it is
    if (pos.due_up() != mySide) {
        return -score;
    }
    return score;
}




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

// le fishe
int main()
{
    std::string line;
    /* read input board state */
    while (std::getline(std::cin, line)) {
        Position pos(line);
        MoveList moves(pos);

        if (pos.time_left() < -1.0) {
            continue;
        }

        uint64_t initial_key = compute_initial_key(pos);
        Color mySide = pos.due_up();

        // If our actual side is Black, we need to reflect this in the TT key
        if (mySide == Black) {
            initial_key ^= MY_SIDE_ZOBRIST_KEY;
        }

        int chosen = -1;
        float best_score = -std::numeric_limits<float>::infinity();
        int search_depth = 8; // Adjust depth as needed

        for (int i = 0; i < moves.size(); ++i) {
            float current_score;
            Move current_move = moves[i];

            if (current_move.type() == Flipping) {
                 // For root flips, we also need to average over possibilities
                 current_score = Star0_5_EQU_F(pos, initial_key, current_move, search_depth, -std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity(), mySide);
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
                current_score = G4_NegaScout(next_pos, next_key, search_depth - 1, -std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity(), mySide);
            }
            
            if (current_score > best_score) {
                best_score = current_score;
                chosen = i;
            }
            debug << "Move " << i << " score: " << current_score << "\n";
            std::fflush(stderr);
        }
        
        debug << "TT Stats: Hits=" << tt_hits 
              << ", Probes=" << tt_probes 
              << ", Rate=" << (tt_probes > 0 ? (double)tt_hits / tt_probes * 100.0 : 0.0) << "%\n";

        if (chosen != -1) {
            /* output the move */
            info << moves[chosen];
        } else {
            // Fallback: if no move was chosen (e.g. all moves lead to loss), play a random one.
            info << strategy_random(moves);
        }
    }
}

