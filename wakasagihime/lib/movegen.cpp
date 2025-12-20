// Chinese Dark Chess: move generation
// ----------------------------------
// This is literally Stockfish

#include "movegen.h"
#include "chess.h"
#include "types.h"

Move *generate_moves(const Color Us, const PieceType pt, const Position &pos, Move *moveList)
{
    assert(pt < REAL_PIECE_TYPE_NB && (Us == Red || Us == Black || Us == Mystery));

    Board bb = pt == Hidden ? pos.pieces(Hidden) : pos.pieces(Us, pt);
    if (bb == 0) {
        return moveList;
    }

    Board pieces = pos.pieces();
    Board target = pos.subordinates(Us, pt) | ~pieces;
    for (Square from : BoardView(bb)) {
        if (pt == Hidden) {
            // flip
            *moveList++ = Move(from, from);
        } else {
            // move
            Board attacks = attacks_bb(pt, from, pieces) & target;
            for (Square to : BoardView(attacks)) {
                *moveList++ = Move(from, to);
            }
        }
    }
    return moveList;
}

template<MoveType Type>
Move *generate_all(const Color Us, const Position &pos, Move *moveList)
{
    assert(Us == Color::Black || Us == Color::Red);

    constexpr bool make_moves = (Type & Moving);
    constexpr bool make_flips = (Type & Flipping);

    if (make_moves) {
        moveList = generate_moves(Us, General, pos, moveList);
        moveList = generate_moves(Us, Advisor, pos, moveList);
        moveList = generate_moves(Us, Elephant, pos, moveList);
        moveList = generate_moves(Us, Chariot, pos, moveList);
        moveList = generate_moves(Us, Horse, pos, moveList);
        moveList = generate_moves(Us, Cannon, pos, moveList);
        moveList = generate_moves(Us, Soldier, pos, moveList);
    }
    if (make_flips) {
        moveList = generate_moves(Us, Hidden, pos, moveList);
    }

    return moveList;
}

// Explicit template instantiation
template Move *generate_all<Moving>(Color, const Position &, Move *);
template Move *generate_all<Flipping>(Color, const Position &, Move *);
template Move *generate_all<All>(Color, const Position &, Move *);

template<MoveType Type, Color Side>
Move *generate(const Position &pos, PieceType pieceType, Move *moveList)
{
    assert(pieceType < SHOWN_PIECE_TYPE_NB || pieceType == ALL_PIECES);

    if (pieceType == ALL_PIECES) {
        if (Side == Mystery) {
            return generate_all<Type>(pos.due_up(), pos, moveList);
        } else {
            return generate_all<Type>(Side, pos, moveList);
        }
    } else {
        if (Side == Mystery) {
            return generate_moves(pos.due_up(), pieceType, pos, moveList);
        } else {
            return generate_moves(Side, pieceType, pos, moveList);
        }
    }
}

// Template aaaa
template Move *generate<All, Mystery>(const Position &, PieceType pieceType, Move *);
template Move *generate<All, Red>(const Position &, PieceType pieceType, Move *);
template Move *generate<All, Black>(const Position &, PieceType pieceType, Move *);
template Move *generate<Moving, Mystery>(const Position &, PieceType pieceType, Move *);
template Move *generate<Moving, Black>(const Position &, PieceType pieceType, Move *);
template Move *generate<Moving, Red>(const Position &, PieceType pieceType, Move *);
template Move *generate<Flipping, Mystery>(const Position &, PieceType pieceType, Move *);
template Move *generate<Flipping, Red>(const Position &, PieceType pieceType, Move *);
template Move *generate<Flipping, Black>(const Position &, PieceType pieceType, Move *);