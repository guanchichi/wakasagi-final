// Chinese Dark Chess: move generation
// ----------------------------------
// This is literally Stockfish

#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "types.h"
#include <algorithm>

// stockfish uses 256, so it's probably enough
constexpr int MAX_MOVES = 256;

/*
 * Generate legal moves.
 * @internal
 * @note    Use the MoveList class directly.
 */
template<MoveType, Color>
Move *generate(const Position &pos, PieceType pieceType, Move *moveList);

template<MoveType T = All, Color C = Mystery>
class MoveList {
    private:
    Move moveList[MAX_MOVES], *last;

    public:
    /*
     * Generates and stores legal moves.
     *
     * @param   T   Type of moves to generate. Ignored if _pt_ is specified.
     *              Defaults to All.
     *              Options: {All, Moving, Flipping}
     * @param   C   Color whose moves to generate.
     *              Defaults to the side to play.
     * @param   pos The position whose moves to generate.
     * @param   pt  Only generate moves for this piece type.
     *              Defaults to ALL_PIECES if not specified.
     *              _T_ is ignored if specified.
     */
    explicit MoveList(const Position &pos)
      : last(generate<T, C>(pos, ALL_PIECES, moveList))
    {}
    // If a piece type is specified, MoveType is ignored
    explicit MoveList(const Position &pos, PieceType pt)
      : last(generate<T, C>(pos, pt, moveList))
    {}

    /*
     * Generate moves and appends them to this MoveList.
     * The same moves may be added again, to remove them use deduplicate() below.
     * The capacity is 256 moves, exceeding this number is undefined behaviour.
     * (probably a segmentation fault)
     *
     * @param   pos   Same as above.
     * @param   pt    Same as above.
     * @note    Template params _T_ and _C_ are decided when you created this MoveList.
     */
    void extend_moves(const Position &pos, PieceType pt)
    {
        last = generate<T, C>(pos, pt, (Move *)end());
    }

    /*
     * Removes duplicate moves from this MoveList.
     * This changes the orders of the moves arbitrarily.
     * @note    This showcases the ability to use functions like std::sort
     *          on MoveList, for more examples see lib/helper.h
     */
    void deduplicate()
    {
        std::sort(this->begin(), this->end());
        last = std::unique(this->begin(), this->end());
    }

    /*
     * Index the MoveList like an array.
     */
    constexpr Move operator[](size_t index) { return moveList[index]; }

    Move *begin() { return moveList; }
    Move *end() { return last; }
    const Move *begin() const { return moveList; }
    const Move *end() const { return last; }
    size_t size() const { return last - moveList; }
};

#endif