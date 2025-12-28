#include "star.h"
#include "negascout.h"
#include "chess.h"
#include "types.h"
#include "helper.h"
#include "zobrist.h"
#include "evaluate.h"
#include <vector>
#include <algorithm>

// uper and lower bounds for Star1 algorithm
const float V_MIN = -60000.0f;
const float V_MAX = 60000.0f;

float Star1_EQU_F(Position& pos, uint64_t key, Move flip_move, int depth, float alpha, float beta, Color mySide) {
    Piece p = pos.peek_piece_at(flip_move.from());
    if (p.side != Mystery) {
        return evaluate(pos, mySide);
    }

    // Horizon node
    if (depth <= 0) {
        return get_flip_score(pos, flip_move.from(), mySide);
    }

    auto potential_pieces = pos.get_collection(); 
    int c = potential_pieces.size(); 

    if (c == 0) {
        Position next_pos = pos;
        next_pos.do_move(flip_move);
        return G4_NegaScout(next_pos, key, depth - 1, alpha, beta, mySide);
    }

    
    Sort potential pieces by their values (high to low)
    std::sort(potential_pieces.begin(), potential_pieces.end(), [](const Piece& a, const Piece& b) {
        return piece_value(a.type) > piece_value(b.type);
    });

    // --- Star1 initialization ---
    float A = (float)c * (alpha - V_MAX) + V_MAX;
    float B = (float)c * (beta - V_MIN) + V_MIN;

    float m_i = V_MIN; 
    float M_i = V_MAX; 
    float vsum = 0;    

    for (int i = 0; i < c; ++i) {
        Piece piece_outcome = potential_pieces[i]; 
        std::vector <Piece> remaining_pieces = potential_pieces;
        remaining_pieces.erase(remaining_pieces.begin() + i);

        Position next_pos(pos); 
        
        next_pos.clear_collection();
        next_pos.add_collection(&piece_outcome, 1);
        
        uint64_t next_key = key;
        Piece hidden_piece(Mystery, Hidden);
        update_key(next_key, flip_move.from(), piece_to_index(hidden_piece)); 
        update_key(next_key, flip_move.from(), piece_to_index(piece_outcome)); 
        
        next_pos.do_move(flip_move);

        if (!remaining_pieces.empty()) {
            next_pos.add_collection(remaining_pieces.data(), remaining_pieces.size());
        }

        Color next_mySide = mySide;
        if (piece_outcome.side != mySide && pos.peek_piece_at(flip_move.from()).side == Mystery) {
            next_mySide = ~mySide;
            next_key ^= MY_SIDE_ZOBRIST_KEY;
        }

        // --- Star1 window ---
        float child_alpha = std::max(A, V_MIN);
        float child_beta  = std::min(B, V_MAX);

        float t = G4_NegaScout(next_pos, next_key, depth - 1, child_alpha, child_beta, next_mySide);

        m_i = m_i + (t - V_MIN) / c;
        M_i = M_i + (t - V_MAX) / c;

        // --- Star1 Cutoff ---
        // Fail High (Beta cut)
        if (t >= B) return m_i; 
        
        // Fail Low (Alpha cut)
        if (t <= A) return M_i; 

        vsum += t;

        // --- Star1 Update ---
        A += V_MAX - t;
        B += V_MIN - t;
    }

    return vsum / c;
}

float Star1_EQU_G(Position& pos, uint64_t key, Move flip_move, int depth, float alpha, float beta, Color mySide) {
    Piece p = pos.peek_piece_at(flip_move.from());
    if (p.side != Mystery) { 
        return evaluate(pos, mySide);
    }

    if (depth <= 0) {
        return get_flip_score(pos, flip_move.from(), mySide);
    }
    
    auto potential_pieces = pos.get_collection();
    int c = potential_pieces.size();

    if (c == 0) {
        Position next_pos = pos;
        next_pos.do_move(flip_move);
        return F4_NegaScout(next_pos, key, depth - 1, alpha, beta, mySide);
    }

    Sort potential pieces by their values (high to low)
    std::sort(potential_pieces.begin(), potential_pieces.end(), [](const Piece& a, const Piece& b) {
        return piece_value(a.type) > piece_value(b.type);
    });

    // --- Star1 initialization ---
    float A = (float)c * (alpha - V_MAX) + V_MAX;
    float B = (float)c * (beta - V_MIN) + V_MIN;

    float m_i = V_MIN; 
    float M_i = V_MAX;
    float vsum = 0;

    for (int i = 0; i < c; ++i) {
        Piece piece_outcome = potential_pieces[i];

        std::vector<Piece> remaining_pieces = potential_pieces;
        remaining_pieces.erase(remaining_pieces.begin() + i);

        Position next_pos(pos); 
        next_pos.clear_collection();
        next_pos.add_collection(&piece_outcome, 1);

        uint64_t next_key = key;
        Piece hidden_piece(Mystery, Hidden);
        update_key(next_key, flip_move.from(), piece_to_index(hidden_piece)); 
        update_key(next_key, flip_move.from(), piece_to_index(piece_outcome)); 

        next_pos.do_move(flip_move); 

        if (!remaining_pieces.empty()) {
            next_pos.add_collection(remaining_pieces.data(), remaining_pieces.size());
        }

        Color next_mySide = mySide;
        if (piece_outcome.side != mySide && pos.peek_piece_at(flip_move.from()).side == Mystery) {
            next_mySide = ~mySide;
            next_key ^= MY_SIDE_ZOBRIST_KEY;
        }

        // --- Star1 Window ---
        float child_alpha = std::max(A, V_MIN);
        float child_beta  = std::min(B, V_MAX);

        float t = F4_NegaScout(next_pos, next_key, depth - 1, child_alpha, child_beta, next_mySide);

        m_i = m_i + (t - V_MIN) / c;
        M_i = M_i + (t - V_MAX) / c;

        // --- Star1 Cutoff ---
        if (t >= B) return m_i; 
        if (t <= A) return M_i; 

        vsum += t;

        // --- Star1 Update ---
        A += V_MAX - t;
        B += V_MIN - t;
    }

    return vsum / c;
}