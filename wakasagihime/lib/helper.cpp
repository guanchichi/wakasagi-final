// Chinese Dark Chess
// ----------------------------------
// Useful stuff

#include "helper.h"

extern "C" bool _square_sort_cmp_default(Position &pos, const Square &lhs, const Square &rhs)
{
    // enforce total ordering by casting to int
    return static_cast<int>(pos.peek_piece_at(lhs).type) <
           static_cast<int>(pos.peek_piece_at(rhs).type);
}

// Overridable!
extern "C" bool square_sort_cmp(Position &pos, const Square &lhs, const Square &rhs)
    __attribute__((weak, alias("_square_sort_cmp_default")));

std::vector<Square> squares_sorted(Position &pos, Board b)
{
    auto v = BoardView(b).to_vector();
    std::sort(v.begin(), v.end(), [&](const auto &lhs, const auto &rhs) {
        return square_sort_cmp(pos, lhs, rhs);
    });
    return v;
}

Move strategy_random(MoveList<> &moves)
{
    return moves[rng(moves.size())];
}