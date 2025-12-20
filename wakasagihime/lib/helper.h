// Chinese Dark Chess
// ----------------------------------
// Showing off what you can do with BoardView, mostly

#include "chess.h"
#include "types.h"
#include <algorithm>

/*
 * Returns a vector of all Squares set in a Board,
 * sorted by the rank of the piece on it.
 * (general > advisor > ... > soldier > duck > hidden > empty)
 *
 * You can write your own version of this function
 * or you can override the comparator function used here by defining your own
 *
 * extern "C" bool square_sort_cmp(Position &pos, const Square &lhs, const Square &rhs)
 *
 * in any file, outside lib/, of course.
 *
 * @see lib/helper.cpp
 */
std::vector<Square> squares_sorted(Position &, Board);


Move strategy_random(MoveList<> &moves);