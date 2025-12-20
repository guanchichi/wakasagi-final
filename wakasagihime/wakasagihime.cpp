// Wakasagihime
// Plays Chinese Dark Chess (Banqi)!

#include "lib/chess.h"
#include "lib/marisa.h"
#include "lib/types.h"
#include "lib/helper.h"
#include <vector>
#include <algorithm>
#include <limits>

// Forward declarations for recursive functions
float F4_NegaScout(Position& pos, int depth, float alpha, float beta);
float G4_NegaScout(Position& pos, int depth, float alpha, float beta);
float Star0_5_EQU_F(Position& pos, Move flip_move, int depth, float alpha, float beta);
float Star0_5_EQU_G(Position& pos, Move flip_move, int depth, float alpha, float beta);

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

float evaluate(const Position& pos) {
    float score = 0;
    Color us = pos.due_up();
    Color them = ~us;

    for (PieceType pt = General; pt < SHOWN_PIECE_TYPE_NB; pt += 1) {
        score += pos.count(us, pt) * piece_value(pt);
        score -= pos.count(them, pt) * piece_value(pt);
    }
    return score;
}

float Star0_5_EQU_F(Position& pos, Move flip_move, int depth, float alpha, float beta) {
    auto potential_pieces = pos.get_collection(); 
    int c = potential_pieces.size(); 

    if (c == 0) {
        Position next_pos = pos;
        next_pos.do_move(flip_move);
        return G4_NegaScout(next_pos, depth - 1, alpha, beta);
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
        next_pos.do_move(flip_move); 

        float t = G4_NegaScout(next_pos, depth - 1, v_min, v_max);

        m_i = m_i + (t - v_min) / c;
        M_i = M_i + (t - v_max) / c;

        if (m_i >= beta) return m_i; 
        if (M_i <= alpha) return M_i;

        vsum += t;
    }

    return vsum / c;
}

float Star0_5_EQU_G(Position& pos, Move flip_move, int depth, float alpha, float beta) {
    auto potential_pieces = pos.get_collection();
    int c = potential_pieces.size();
    if (c == 0) {
        Position next_pos = pos;
        next_pos.do_move(flip_move);
        return F4_NegaScout(next_pos, depth - 1, alpha, beta);
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
        next_pos.do_move(flip_move); 

        float t = F4_NegaScout(next_pos, depth - 1, v_min, v_max);

        m_i = m_i + (t - v_min) / c;
        M_i = M_i + (t - v_max) / c;

        if (m_i >= beta) return m_i;
        if (M_i <= alpha) return M_i;

        vsum += t;
    }

    return vsum / c;
}


float F4_NegaScout(Position& pos, int depth, float alpha, float beta) {
    if (depth == 0 || pos.winner() != NO_COLOR) {
        return evaluate(pos);
    }

    MoveList moves(pos);
    if (moves.size() == 0) {
        return evaluate(pos);
    }

    float best_score = -std::numeric_limits<float>::infinity();

    // First move with full window
    Move first_move = moves[0];
    Position first_pos = pos;
    float score;

    if (first_move.type() == Flipping) {
        score = Star0_5_EQU_F(first_pos, first_move, depth, alpha, beta);
    } else {
        first_pos.do_move(first_move);
        score = G4_NegaScout(first_pos, depth - 1, alpha, beta);
    }
    
    best_score = std::max(best_score, score);
    alpha = std::max(alpha, best_score);
    
    if (alpha >= beta) {
        return best_score;
    }

    // Subsequent moves with null window
    for (size_t i = 1; i < moves.size(); ++i) {
        Position next_pos = pos;
        Move move = moves[i];
        
        if (move.type() == Flipping) {
            score = Star0_5_EQU_F(next_pos, move, depth, alpha, alpha + 1);
        } else {
            next_pos.do_move(move);
            score = G4_NegaScout(next_pos, depth - 1, alpha, alpha + 1);
        }

        if (score > alpha && score < beta) { // Re-search
            Position research_pos = pos;
            if (move.type() == Flipping) {
                 score = Star0_5_EQU_F(research_pos, move, depth, alpha, beta);
            } else {
                 research_pos.do_move(move);
                 score = G4_NegaScout(research_pos, depth - 1, alpha, beta);
            }
        }
        
        best_score = std::max(best_score, score);
        alpha = std::max(alpha, best_score);
        
        if (alpha >= beta) {
            break;
        }
    }

    return best_score;
}

float G4_NegaScout(Position& pos, int depth, float alpha, float beta) {
    if (depth == 0 || pos.winner() != NO_COLOR) {
        return evaluate(pos);
    }

    MoveList moves(pos);
    if (moves.size() == 0) {
        return evaluate(pos);
    }

    float best_score = std::numeric_limits<float>::infinity();

    // First move with full window
    Move first_move = moves[0];
    Position first_pos = pos;
    float score;
    
    if (first_move.type() == Flipping) {
        score = Star0_5_EQU_G(first_pos, first_move, depth, alpha, beta);
    } else {
        first_pos.do_move(first_move);
        score = F4_NegaScout(first_pos, depth - 1, alpha, beta);
    }
    
    best_score = std::min(best_score, score);
    beta = std::min(beta, best_score);

    if (alpha >= beta) {
        return best_score;
    }

    // Subsequent moves with null window
    for (size_t i = 1; i < moves.size(); ++i) {
        Position next_pos = pos;
        Move move = moves[i];

        if (move.type() == Flipping) {
            score = Star0_5_EQU_G(next_pos, move, depth, beta - 1, beta);
        } else {
            next_pos.do_move(move);
            score = F4_NegaScout(next_pos, depth - 1, beta - 1, beta);
        }
        
        if (score < beta && score > alpha) { // Re-search
             Position research_pos = pos;
             if (move.type() == Flipping) {
                 score = Star0_5_EQU_G(research_pos, move, depth, alpha, beta);
             } else {
                 research_pos.do_move(move);
                 score = F4_NegaScout(research_pos, depth - 1, alpha, beta);
             }
        }
        
        best_score = std::min(best_score, score);
        beta = std::min(beta, best_score);
        
        if (alpha >= beta) {
            break;
        }
    }

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

        int chosen = -1;
        float best_score = -std::numeric_limits<float>::infinity();
        int search_depth = 4; // Adjust depth as needed

        for (int i = 0; i < moves.size(); ++i) {
            Position next_pos = pos;
            float current_score;
            Move current_move = moves[i];

            if (current_move.type() == Flipping) {
                 // For root flips, we also need to average over possibilities
                 current_score = Star0_5_EQU_F(next_pos, current_move, search_depth, -std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity());
            } else {
                next_pos.do_move(current_move);
                current_score = G4_NegaScout(next_pos, search_depth - 1, -std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity());
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
