#include "star.h"
#include "negascout.h"
#include "chess.h"
#include "types.h"
#include "helper.h"
#include "zobrist.h"
#include "evaluate.h"
#include <vector>



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
