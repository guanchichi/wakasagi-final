#include "negascout.h"
#include "star.h"
#include "chess.h"
#include "types.h"
#include "helper.h"
#include "zobrist.h"
#include "tt.h"
#include "evaluate.h"
#include <vector>
#include <algorithm>
#include <limits>




float F4_NegaScout(Position& pos, uint64_t key, int depth, float alpha, float beta, Color mySide) {
    float original_alpha = alpha;
    float score = 0.0f;
    Move tt_move;

    // 1. Probe Transposition Table
    if (tt_probe(key, depth, alpha, beta, score, tt_move)) {
        return score;
    }

    if (depth == 0 || pos.winner() != NO_COLOR) {
        return evaluate(pos, mySide);
    }

    MoveList moves(pos);
    if (moves.size() == 0) {
        return evaluate(pos, mySide);
    }

    // Move Ordering: Prioritize the move from the TT
    for (size_t i = 0; i < moves.size(); ++i) {
        if (moves[i] == tt_move) {
            Move temp = moves[0];
            moves[0] = moves[i];
            moves[i] = temp;
            break;
        }
    }

    float best_score = -std::numeric_limits<float>::infinity();
    Move best_move_for_node;

    // First move with full window
    {
        Move move = moves[0];
        if (move.type() == Flipping) {
            score = Star0_5_EQU_F(pos, key, move, depth, alpha, beta, mySide);
        } else {
            Position next_pos = pos;
            uint64_t next_key = key;
            Piece moving_piece = next_pos.peek_piece_at(move.from());
            Piece captured_piece = next_pos.peek_piece_at(move.to());

            update_key(next_key, move.from(), piece_to_index(moving_piece));
            if (captured_piece.type != NO_PIECE) {
                update_key(next_key, move.to(), piece_to_index(captured_piece));
            }
            update_key(next_key, move.to(), piece_to_index(moving_piece));
            
            next_key ^= MY_SIDE_ZOBRIST_KEY;

            next_pos.do_move(move);
            score = G4_NegaScout(next_pos, next_key, depth - 1, alpha, beta, mySide);
        }
        
        if (score > best_score) {
            best_score = score;
            best_move_for_node = move;
        }
        alpha = std::max(alpha, best_score);
        
        if (alpha >= beta) {
            tt_store(key, depth, best_score, FLAG_LOWER_BOUND, best_move_for_node);
            return best_score;
        }
    }

    // Subsequent moves with null window
    for (size_t i = 1; i < moves.size(); ++i) {
        Move move = moves[i];
        
        if (move.type() == Flipping) {
            score = Star0_5_EQU_F(pos, key, move, depth, alpha, alpha + 1, mySide);
        } else {
            Position next_pos = pos;
            uint64_t next_key = key;
            Piece moving_piece = next_pos.peek_piece_at(move.from());
            Piece captured_piece = next_pos.peek_piece_at(move.to());

            update_key(next_key, move.from(), piece_to_index(moving_piece));
            if (captured_piece.type != NO_PIECE) {
                update_key(next_key, move.to(), piece_to_index(captured_piece));
            }
            update_key(next_key, move.to(), piece_to_index(moving_piece));

            next_key ^= MY_SIDE_ZOBRIST_KEY;

            next_pos.do_move(move);
            score = G4_NegaScout(next_pos, next_key, depth - 1, alpha, alpha + 1, mySide);
        }

        if (score > alpha && score < beta) { // Re-search
            if (move.type() == Flipping) {
                 score = Star0_5_EQU_F(pos, key, move, depth, alpha, beta, mySide);
            } else {
                 Position research_pos = pos;
                 uint64_t next_key = key;
                 Piece moving_piece = research_pos.peek_piece_at(move.from());
                 Piece captured_piece = research_pos.peek_piece_at(move.to());

                 update_key(next_key, move.from(), piece_to_index(moving_piece));
                 if (captured_piece.type != NO_PIECE) {
                    update_key(next_key, move.to(), piece_to_index(captured_piece));
                 }
                 update_key(next_key, move.to(), piece_to_index(moving_piece));

                 next_key ^= MY_SIDE_ZOBRIST_KEY;
                 
                 research_pos.do_move(move);
                 score = G4_NegaScout(research_pos, next_key, depth - 1, alpha, beta, mySide);
            }
        }
        
        if (score > best_score) {
            best_score = score;
            best_move_for_node = move;
        }
        alpha = std::max(alpha, best_score);
        
        if (alpha >= beta) {
            break;
        }
    }

    // 2. Store result in Transposition Table
    TTFlag flag = (best_score > original_alpha) ? FLAG_EXACT : FLAG_UPPER_BOUND;
    if (best_score >= beta) flag = FLAG_LOWER_BOUND;
    tt_store(key, depth, best_score, flag, best_move_for_node);

    return best_score;
}

float G4_NegaScout(Position& pos, uint64_t key, int depth, float alpha, float beta, Color mySide) {
    float original_beta = beta;
    float score = 0.0f;
    Move tt_move;

    if (tt_probe(key, depth, alpha, beta, score, tt_move)) {
        return score;
    }

    if (depth == 0 || pos.winner() != NO_COLOR) {
        return evaluate(pos, mySide);
    }

    MoveList moves(pos);
    if (moves.size() == 0) {
        return evaluate(pos, mySide);
    }

    // Move Ordering: Prioritize the move from the TT
    for (size_t i = 0; i < moves.size(); ++i) {
        if (moves[i] == tt_move) {
            Move temp = moves[0];
            moves[0] = moves[i];
            moves[i] = temp;
            break;
        }
    }

    float best_score = std::numeric_limits<float>::infinity();
    Move best_move_for_node;

    // First move with full window
    {
        Move move = moves[0];
        if (move.type() == Flipping) {
            score = Star0_5_EQU_G(pos, key, move, depth, alpha, beta, mySide);
        } else {
            Position next_pos = pos;
            uint64_t next_key = key;
            Piece moving_piece = next_pos.peek_piece_at(move.from());
            Piece captured_piece = next_pos.peek_piece_at(move.to());

            update_key(next_key, move.from(), piece_to_index(moving_piece));
            if (captured_piece.type != NO_PIECE) {
                update_key(next_key, move.to(), piece_to_index(captured_piece));
            }
            update_key(next_key, move.to(), piece_to_index(moving_piece));

            next_key ^= MY_SIDE_ZOBRIST_KEY;

            next_pos.do_move(move);
            score = F4_NegaScout(next_pos, next_key, depth - 1, alpha, beta, mySide);
        }
        
        if (score < best_score) {
            best_score = score;
            best_move_for_node = move;
        }
        beta = std::min(beta, best_score);

        if (alpha >= beta) {
            tt_store(key, depth, best_score, FLAG_UPPER_BOUND, best_move_for_node);
            return best_score;
        }
    }

    // Subsequent moves with null window
    for (size_t i = 1; i < moves.size(); ++i) {
        Move move = moves[i];

        if (move.type() == Flipping) {
            score = Star0_5_EQU_G(pos, key, move, depth, beta - 1, beta, mySide);
        } else {
            Position next_pos = pos;
            uint64_t next_key = key;
            Piece moving_piece = next_pos.peek_piece_at(move.from());
            Piece captured_piece = next_pos.peek_piece_at(move.to());

            update_key(next_key, move.from(), piece_to_index(moving_piece));
            if (captured_piece.type != NO_PIECE) {
                update_key(next_key, move.to(), piece_to_index(captured_piece));
            }
            update_key(next_key, move.to(), piece_to_index(moving_piece));

            next_key ^= MY_SIDE_ZOBRIST_KEY;
            
            next_pos.do_move(move);
            score = F4_NegaScout(next_pos, next_key, depth - 1, beta - 1, beta, mySide);
        }
        
        if (score < beta && score > alpha) { // Re-search
             if (move.type() == Flipping) {
                 score = Star0_5_EQU_G(pos, key, move, depth, alpha, beta, mySide);
             } else {
                 Position research_pos = pos;
                 uint64_t next_key = key;
                 Piece moving_piece = research_pos.peek_piece_at(move.from());
                 Piece captured_piece = research_pos.peek_piece_at(move.to());

                 update_key(next_key, move.from(), piece_to_index(moving_piece));
                 if (captured_piece.type != NO_PIECE) {
                    update_key(next_key, move.to(), piece_to_index(captured_piece));
                 }
                 update_key(next_key, move.to(), piece_to_index(moving_piece));

                 next_key ^= MY_SIDE_ZOBRIST_KEY;

                 research_pos.do_move(move);
                 score = F4_NegaScout(research_pos, next_key, depth - 1, alpha, beta, mySide);
             }
        }
        
        if (score < best_score) {
            best_score = score;
            best_move_for_node = move;
        }
        beta = std::min(beta, best_score);
        
        if (alpha >= beta) {
            break;
        }
    }

    TTFlag flag = (best_score < original_beta) ? FLAG_EXACT : FLAG_LOWER_BOUND;
    if (best_score <= alpha) flag = FLAG_UPPER_BOUND;
    tt_store(key, depth, best_score, flag, best_move_for_node);

    return best_score;
}
