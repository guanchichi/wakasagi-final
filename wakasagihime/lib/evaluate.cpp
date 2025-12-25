// lib/evaluate.cpp

#include "evaluate.h"
#include "chess.h"
#include "types.h"
// #include <cmath> // No longer needed

// --- Precomputed table for distance-based score decay ---
// Replaces std::pow(2.0f, 2.0f - md)
static const float DecayTable[12] = {
    // md = 0, 1 are unused and should not be accessed
    0.0f, 0.0f,
    1.0f,      // md = 2: 2^0
    0.5f,      // md = 3: 2^-1
    0.25f,     // md = 4: 2^-2
    0.125f,    // md = 5: 2^-3
    0.0625f,   // md = 6: 2^-4
    0.03125f,  // md = 7: 2^-5
    0.015625f, // md = 8: 2^-6
    0.0078125f, // md = 9: 2^-7
    0.00390625f, // md = 10: 2^-8
    0.001953125f // md = 11: 2^-9
};

// --- Forward declarations for functions used within this file ---
float get_flip_score(const Position& pos, Square sq, Color mySide);
int piece_value(PieceType pt);
bool is_dominating(const Position& pos, Color mySide);


// --- Helper function for move ordering ---
float calculate_influence_at(Square s, const Position& pos) {
    float total_influence = 0;
    Piece p = pos.peek_piece_at(s);

    if (p.type >= MOVABLE_PIECE_TYPE_NB) {
        return 0;
    }

    Board all_pieces = pos.pieces(ALL_PIECES) & ~pos.pieces(Hidden);
    for (Square other_sq : BoardView(all_pieces)) {
        if (s == other_sq) continue;
        Piece other_p = pos.peek_piece_at(other_sq);
        if (other_p.type >= MOVABLE_PIECE_TYPE_NB) continue;
        
        if (p.side == other_p.side) continue;

        int md = distance<Square>(s, other_sq);
        float i = InfluenceValues[p.type][other_p.type];
        float w = 0;

        if (md == 1) {
            w = i / 2.0f;
        } else if (md > 1 && md < 12) { // Use lookup table
            w = i * DecayTable[md];
        }
        total_influence += w;
    }
    return total_influence;
}


int piece_value(PieceType pt) {
    switch (pt) {
        case General:  return 5500;
        case Advisor:  return 5000;
        case Elephant: return 2500;
        case Chariot:  return 1000;
        case Horse:    return 800;
        case Cannon:   return 3000;
        case Soldier:  return 800;
        default:       return 0;
    }
}

int get_move_score(const Move& m, const Position& pos, Color mySide) {
    if (m.type() == Moving) {
        Piece attacker = pos.peek_piece_at(m.from());
        Piece victim = pos.peek_piece_at(m.to());

        // 1. Capture Moves (MVV-LVA)
        if (victim.type != NO_PIECE) {
            if (attacker.side == victim.side) return -999999; 

            int victim_value = piece_value(victim.type);
            int attacker_value = piece_value(attacker.type);
            return (victim_value * 10) - attacker_value + 100000;
        } 
        // 2. Non-Capture Moves (Influence Gain)
        else {
            float influence_gain = 0;
            Board all_pieces = pos.pieces(ALL_PIECES) & ~pos.pieces(Hidden);
            Piece moving_piece = pos.peek_piece_at(m.from());
            if(moving_piece.type >= MOVABLE_PIECE_TYPE_NB) return 0;

            for (Square other_sq : BoardView(all_pieces)) {
                if (other_sq == m.from() || other_sq == m.to()) continue;
                Piece other_p = pos.peek_piece_at(other_sq);
                if (other_p.type >= MOVABLE_PIECE_TYPE_NB || moving_piece.side == other_p.side) continue;

                float i_val = InfluenceValues[moving_piece.type][other_p.type];
                
                int md_dest = distance<Square>(m.to(), other_sq);
                float w_dest = 0;
                if (md_dest == 1) w_dest = i_val / 2.0f;
                else if (md_dest > 1 && md_dest < 12) w_dest = i_val * DecayTable[md_dest];

                int md_src = distance<Square>(m.from(), other_sq);
                float w_src = 0;
                if (md_src == 1) w_src = i_val / 2.0f;
                else if (md_src > 1 && md_src < 12) w_src = i_val * DecayTable[md_src];

                influence_gain += (w_dest - w_src);
            }
            return static_cast<int>(influence_gain);
        }
    }
    // 3. Flipping Moves
    else if (m.type() == Flipping) {
        return static_cast<int>(get_flip_score(pos, m.from(), mySide));
    }
    return 0;
}

// --- Endgame Chase Bonus Helpers ---

// Helper to find the best (highest rank) piece type for a side
PieceType get_best_piece_type(const Position& pos, Color side) {
    for (PieceType pt = General; pt < MOVABLE_PIECE_TYPE_NB; pt = PieceType(pt + 1)) {
        if (pos.count(side, pt) > 0) {
            return pt;
        }
    }
    return NO_PIECE;
}

bool is_dominating(const Position& pos, Color mySide) {
    Color them = ~mySide;
    
    Board their_faceup_pieces = pos.pieces(them) & ~pos.pieces(Hidden);
    int their_piece_count = 0;
    for (Square sq : BoardView(their_faceup_pieces)) {
        their_piece_count++;
    }

    // Condition 1: Enemy has few pieces (<= 2)
    if (their_piece_count > 2) {
        return false;
    }

    PieceType my_best_pt = get_best_piece_type(pos, mySide);
    PieceType their_best_pt = get_best_piece_type(pos, them);

    if (my_best_pt == NO_PIECE) return false; // We have no pieces, can't dominate
    if (their_best_pt == NO_PIECE) return true; // They have no pieces, clear win

    // Condition 2: Rank Dominance (lower enum value is higher rank)
    if (my_best_pt > their_best_pt) {
        return false;
    }

    // Exception 1: King vs Pawn
    if (my_best_pt == General && pos.count(them, Soldier) > 0) {
        Board my_faceup_pieces = pos.pieces(mySide) & ~pos.pieces(Hidden);
        int my_piece_count = 0;
        for (Square sq : BoardView(my_faceup_pieces)) {
            my_piece_count++;
        }
        if (my_piece_count <= 1) { // If we only have the king left
            return false;
        }
    }

    // Exception 2: Lone Cannon
    if (my_best_pt == Cannon) {
        Board all_faceup_pieces = pos.pieces(ALL_PIECES) & ~pos.pieces(Hidden);
        int total_piece_count = 0;
        for (Square sq : BoardView(all_faceup_pieces)) {
            total_piece_count++;
        }
        if (total_piece_count < 3) {
            return false;
        }
    }
    
    return true;
}


float evaluate(const Position& pos, Color mySide) {
    float material_score = 0;
    float influence_score = 0;
    Color us = mySide;
    Color them = ~us;

    // 1. Calculate material score
    for (PieceType pt = General; pt < MOVABLE_PIECE_TYPE_NB; pt = PieceType(pt + 1)) {
        material_score += pos.count(us, pt) * piece_value(pt);
        material_score -= pos.count(them, pt) * piece_value(pt);
    }

    // 2. Calculate influence score
    Board all_faceup_pieces = pos.pieces(ALL_PIECES) & ~pos.pieces(Hidden);
    for (Square attacker_sq : BoardView(all_faceup_pieces)) {
        Piece attacker = pos.peek_piece_at(attacker_sq);
        
        if (attacker.type >= MOVABLE_PIECE_TYPE_NB) continue;

        for (Square defender_sq : BoardView(all_faceup_pieces)) {
            if (attacker_sq == defender_sq) continue;

            Piece defender = pos.peek_piece_at(defender_sq);

            if (defender.type >= MOVABLE_PIECE_TYPE_NB) continue;

            int md = distance<Square>(attacker_sq, defender_sq);
            float i = InfluenceValues[attacker.type][defender.type];
            float w = 0;

            if (md == 1) {
                w = i / 2.0f;
            } else if (md > 1 && md < 12) { // Use lookup table
                w = i * DecayTable[md];
            }
            
            if (attacker.side != defender.side) {
                 if (attacker.side == us) {
                    influence_score += w;
                } else {
                    influence_score -= w;
                }
            }
        }
    }
    
    float total_score = material_score + influence_score;

    // 3. Calculate Chase Bonus in dominating endgame
    if (is_dominating(pos, mySide)) {
        float chase_bonus = 0;
        const int ChaseWeight = 20; // Increased from 10
        Board my_pieces = pos.pieces(us) & ~pos.pieces(Hidden);
        Board their_pieces = pos.pieces(them) & ~pos.pieces(Hidden);
        
        int their_piece_count = 0;
        for (Square sq : BoardView(their_pieces)) { their_piece_count++; }

        if (their_piece_count > 0) {
            for (Square my_sq : BoardView(my_pieces)) {
                int min_dist_to_enemy = 32;
                for (Square their_sq : BoardView(their_pieces)) {
                    min_dist_to_enemy = std::min(min_dist_to_enemy, distance<Square>(my_sq, their_sq));
                }
                chase_bonus += (32 - min_dist_to_enemy) * ChaseWeight;
            }
        }
        total_score += chase_bonus;
    }

    return (pos.due_up() == mySide) ? total_score : -total_score;
}

float get_flip_score(const Position& pos, Square sq, Color mySide) {
    float score = 0;
    Color us = mySide;
    Color them = ~us;
    
    Direction neighbors_dir[4] = {NORTH, SOUTH, EAST, WEST};

    for (Direction dir : neighbors_dir) {
        Square neighbor_sq = sq + dir;

        if (!is_okay(neighbor_sq) || distance<Square>(sq, neighbor_sq) > 1) {
            continue;
        }

        Piece p = pos.peek_piece_at(neighbor_sq);

        if (p.type == NO_PIECE || p.type == Hidden) {
            continue;
        }

        if (p.type >= MOVABLE_PIECE_TYPE_NB) {
            continue;
        }

        if (p.side == them) {
            score += FlippingHeuristicOpponent[p.type];
        } else if (p.side == us) {
            score += FlippingHeuristicMySide[p.type];
        }
    }

    return score;
}