// Wakasagihime
// Plays Chinese Dark Chess (Banqi)!

#include "lib/chess.h"
#include "lib/marisa.h"
#include "lib/types.h"
#include "lib/helper.h"
#include "lib/zobrist.h"
#include "lib/tt.h"
#include <vector>
#include <algorithm>
#include <limits>

// A fixed Zobrist key to XOR into the TT key when the player's side swaps.
const uint64_t MY_SIDE_ZOBRIST_KEY = 0x547275654E616D65ULL; // "TrueName" in ASCII

// Forward declarations for recursive functions
float F4_NegaScout(Position& pos, uint64_t key, int depth, float alpha, float beta, Color mySide);
float G4_NegaScout(Position& pos, uint64_t key, int depth, float alpha, float beta, Color mySide);
float Star0_5_EQU_F(Position& pos, uint64_t key, Move flip_move, int depth, float alpha, float beta, Color mySide);
float Star0_5_EQU_G(Position& pos, uint64_t key, Move flip_move, int depth, float alpha, float beta, Color mySide);

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

float Star0_5_EQU_F(Position& pos, uint64_t key, Move flip_move, int depth, float alpha, float beta, Color mySide) {
    Piece p = pos.peek_piece_at(flip_move.from());
    if (p.side != Mystery) {
        return evaluate(pos, mySide);
    }

    auto potential_pieces = pos.get_collection(); 
    int c = potential_pieces.size(); 

    bool is_first_flip = (pos.count(Hidden) == 32);

    if (c == 0) {
        Position next_pos = pos;
        uint64_t next_key = key;
        // This path is tricky, as the outcome is random and not known.
        // A full implementation might need to pass the key by reference and update it in do_move.
        next_pos.do_move(flip_move);
        return G4_NegaScout(next_pos, next_key, depth - 1, alpha, beta, mySide);
    }

    float v_min = -2000.0f; 
    float v_max = 2000.0f;
    float m_i = v_min; 
    float M_i = v_max; 
    float vsum = 0;    

    for (int i = 0; i < c; ++i) {
        Piece piece_outcome = potential_pieces[i]; 
        Position next_pos(pos); 
        
        next_pos.clear_collection();
        next_pos.add_collection(&piece_outcome, 1);
        
        // Incrementally update key for the flip
        uint64_t next_key = key;
        Piece hidden_piece(Mystery, Hidden);
        update_key(next_key, flip_move.from(), piece_to_index(hidden_piece)); // XOR out hidden
        update_key(next_key, flip_move.from(), piece_to_index(piece_outcome)); // XOR in revealed
        
        next_pos.do_move(flip_move);

        Color next_mySide = mySide;
        if (piece_outcome.side != mySide && pos.peek_piece_at(flip_move.from()).side == Mystery) {
            next_mySide = ~mySide;
            next_key ^= MY_SIDE_ZOBRIST_KEY;
        }

        float t = G4_NegaScout(next_pos, next_key, depth - 1, v_min, v_max, next_mySide);

        m_i = m_i + (t - v_min) / c;
        M_i = M_i + (t - v_max) / c;

        if (m_i >= beta) return m_i; 
        if (M_i <= alpha) return M_i;

        vsum += t;
    }

    return vsum / c;
}

float Star0_5_EQU_G(Position& pos, uint64_t key, Move flip_move, int depth, float alpha, float beta, Color mySide) {
    Piece p = pos.peek_piece_at(flip_move.from());
    if (p.side != Mystery) { 
        return evaluate(pos, mySide);
    }

    
    auto potential_pieces = pos.get_collection();
    int c = potential_pieces.size();
    bool is_first_flip = (pos.count(Hidden) == 32);

    if (c == 0) {
        Position next_pos = pos;
        uint64_t next_key = key;
        next_pos.do_move(flip_move);
        return F4_NegaScout(next_pos, next_key, depth - 1, alpha, beta, mySide);
    }

    float v_min = -2000.0f; 
    float v_max = 2000.0f;
    float m_i = v_min; 
    float M_i = v_max;
    float vsum = 0;

    for (int i = 0; i < c; ++i) {
        Piece piece_outcome = potential_pieces[i];
        Position next_pos(pos); 
        next_pos.clear_collection();
        next_pos.add_collection(&piece_outcome, 1);

        // Incrementally update key for the flip
        uint64_t next_key = key;
        Piece hidden_piece(Mystery, Hidden);
        update_key(next_key, flip_move.from(), piece_to_index(hidden_piece)); // XOR out hidden
        update_key(next_key, flip_move.from(), piece_to_index(piece_outcome)); // XOR in revealed

        next_pos.do_move(flip_move); 

        Color next_mySide = mySide;
        if (piece_outcome.side != mySide && pos.peek_piece_at(flip_move.from()).side == Mystery) {
            next_mySide = ~mySide;
            next_key ^= MY_SIDE_ZOBRIST_KEY;
        }

        float t = F4_NegaScout(next_pos, next_key, depth - 1, v_min, v_max, next_mySide);

        m_i = m_i + (t - v_min) / c;
        M_i = M_i + (t - v_max) / c;

        if (m_i >= beta) return m_i;
        if (M_i <= alpha) return M_i;

        vsum += t;
    }

    return vsum / c;
}


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
        int search_depth = 4; // Adjust depth as needed

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
        
        if (chosen != -1) {
            /* output the move */
            info << moves[chosen];
        } else {
            // Fallback: if no move was chosen (e.g. all moves lead to loss), play a random one.
            info << strategy_random(moves);
        }
    }
}

