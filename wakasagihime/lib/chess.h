// Chinese Dark Chess
// ----------------------------------
// Kind of the main thing

#ifndef CHESS_H
#define CHESS_H

#include "cdc.h"
#include "movegen.h"
#include "types.h"

#include <array>
#include <stack>
#include <cassert>
#include <csignal>
#include <cstdint>
#include <cstring>
#include <optional>

// -~ Colors ~-

/*
 * Invert color.
 * @param   c   The color to invert. Must be RED or BLACK.
 */
inline Color operator~(Color c)
{
    assert(c < 2);
    return Color(c ^ 1);
}

// -~ Squares, Ranks, and Files ~-
extern uint8_t SquareDistance[SQUARE_NB][SQUARE_NB];

std::ostream &operator<<(std::ostream &os, const Square &sq);
std::istream &operator>>(std::istream &is, const Square &sq);

constexpr File &operator+=(File &f, int i)
{
    f = static_cast<File>(static_cast<int>(f) + i);
    return f;
}
constexpr Rank &operator-=(Rank &r, int i)
{
    r = static_cast<Rank>(static_cast<int>(r) - i);
    return r;
}

inline Square make_square(File f, Rank r) { return Square(r * 8 + f); }

constexpr bool is_okay(Square sq) { return (sq >= 0 && sq < 32); }

constexpr int rank_of(Square sq)
{
    assert(is_okay(sq));
    return sq / 8;
}
constexpr int file_of(Square sq)
{
    assert(is_okay(sq));
    return sq % 8;
}

/*
 * Returns distance between squares. Optionally use <Rank/File> to get the difference of
 * ranks/files.
 * @param   x,y Squares to get the distance between.
 */
template<typename T1 = Square>
inline int distance(Square x, Square y);

template<>
inline int distance<File>(Square x, Square y)
{
    return std::abs(file_of(x) - file_of(y));
}
template<>
inline int distance<Rank>(Square x, Square y)
{
    return std::abs(rank_of(x) - rank_of(y));
}
template<>
inline int distance<Square>(Square x, Square y)
{
    return SquareDistance[x][y];
}

/*
 * You can add to Squares with the Direction enum
 * @see lib/types.h
 */
constexpr Square operator+(Square sq, int i)
{
    return static_cast<Square>(static_cast<int>(sq) + i);
}
constexpr Square &operator+=(Square &sq, int i)
{
    sq = static_cast<Square>(static_cast<int>(sq) + i);
    return sq;
}

// -~ Pieces ~-

/*
 * Compares PieceTypes and tells you if a can capture b.
 * @param   a   Capturer piece type
 * @param   b   Target piece type
 */
inline bool operator>(PieceType a, PieceType b)
{
    if (b == Duck) {
        return false; // quack
    }
    if (b == Hidden) {
        return false; // Hidden pieces can't be captured
    }
    if (a == General && b == Soldier) {
        return false; // General can't capture soldiers
    }
    if (a == Soldier && b == General) {
        return true;  // Soldiers can capture generals
    }
    if (a == Cannon) {
        return true;  // Cannons go boom
    }
    return (a <= b);  // Follow generic ranks
}

constexpr PieceType &operator+=(PieceType &sq, int i)
{
    sq = static_cast<PieceType>(static_cast<int>(sq) + i);
    return sq;
}

inline Piece random_faceup_piece()
{
    return Piece(Color(rng(SIDE_NB)), PieceType(rng(MOVABLE_PIECE_TYPE_NB)));
}

// -~ Boards ~-

// Attack bitboards for normal pieces (we only have one type in CDC)
extern Board PseudoAttacks[SQUARE_NB];

// Files & Ranks bitboard constants
constexpr Board FileABB = 0x01010101U;
constexpr Board FileBBB = FileABB << 1;
constexpr Board FileCBB = FileABB << 2;
constexpr Board FileDBB = FileABB << 3;
constexpr Board FileEBB = FileABB << 4;
constexpr Board FileFBB = FileABB << 5;
constexpr Board FileGBB = FileABB << 6;
constexpr Board FileHBB = FileABB << 7;

constexpr Board Rank1BB = 0x000000FF;
constexpr Board Rank2BB = Rank1BB << (8 * 1);
constexpr Board Rank3BB = Rank1BB << (8 * 2);
constexpr Board Rank4BB = Rank1BB << (8 * 3);

/*
 * Files & Ranks bitboard variable
 * @param   rank/file   For a bitboard of a full rank/file
 * @param   sq          For a bitboard of the full rank/file that sq is in
 */
constexpr Board rank_bb(int rank) { return Rank1BB << (8 * rank); }
constexpr Board rank_bb(Square sq) { return rank_bb(rank_of(sq)); }

constexpr Board file_bb(int file) { return FileABB << file; }
constexpr Board file_bb(Square sq) { return file_bb(file_of(sq)); }

/*
 * Square bitboard variable
 * @param   sq  For a bitboard of only sq
 */
constexpr Board square_bb(Square sq)
{
    assert(is_okay(sq));
    return (1UL << sq);
}

/*
 * A pretty string representation of a bitboard that you can output for debugging.
 * @param   b   Bitboard to prettify
 */
std::string pretty(Board &b);

/*
 * Manipulations of bitboards. Use like `b |= s`
 * @param   b   Board to manipulate
 * @param   s   Square to work on
 */
constexpr Board operator&(Board b, Square s) { return b & square_bb(s); }     // get
constexpr Board operator|(Board b, Square s) { return b | square_bb(s); }
constexpr Board operator^(Board b, Square s) { return b ^ square_bb(s); }
constexpr Board &operator|=(Board &b, Square s) { return b |= square_bb(s); } // set
constexpr Board &operator^=(Board &b, Square s) { return b ^= square_bb(s); } // unset

/*
 * Construct a bitboard with 2 squares
 * @param   a,b The 2 squares.
 */
constexpr Board operator|(Square a, Square b) { return square_bb(a) | b; }

/*
 * Attack bitboard (all valid move destinations) for any real face-up piece type
 * @param   pt          Piece type
 * @param   sq          Origin square
 * @param   occupied    All present pieces on the board
 */
template<PieceType pt>
Board attacks_bb(Square sq, Board occupied);
Board attacks_bb(PieceType pt, Square sq, Board occupied);

// -~ Move ~-
std::ostream &operator<<(std::ostream &os, const Move &mv);
std::istream &operator>>(std::istream &is, Move &mv);

// -~ Position ~-
class Position {
    private:
    // Boards
    Piece board[SQUARE_NB];
    Board byTypeBB[PIECE_TYPE_NB];
    Board byColorBB[SIDE_NB];
    // Data
    Color sideToMove;
    std::vector<Piece> pieceCollection;
    StateInfo info;
    std::stack<PastMove> history;

    public:
    /*
     * An empty board.
     */
    Position()
      : sideToMove(Red)
    {
        clear();
    }

    /*
     * A board initialized with a FEN string
     * @param   fen The FEN string
     */
    Position(std::string fen)
    {
        clear();
        readFEN(fen);

        clear_collection();
        add_collection();
    }

    /*
     * Clears the board.
     */
    void clear();

    /*
     * Add some pieces to the bag.
     * When a face-down piece is flipped, it draws from this bag.
     * If the bag is empty, a random piece is generated.
     *
     * @param   set Pointer to the set of pieces to add
     *              If nullptr, the standard Chinese Chess piece set is used
     * @param   n   Number of pieces to add.
     *              Ignored when _set_ is nullptr
     */
    void add_collection(Piece *set = nullptr, size_t n = 0);
    /*
     * Clears the bag for face-down pieces.
     */
    void clear_collection() { pieceCollection.clear(); }
    /*
     * Get all remaining unrevealed pieces.
     */
    std::vector<Piece> get_collection() { return pieceCollection; }

    /*
     * Makes a position from a FEN-like string.
     * @internal
     */
    void readFEN(std::string fen);

    /*
     * Writes the board to a FEN-like string.
     */
    std::string toFEN();

    /*
     * Fills the board with pieces to start.
     * @param   hidden  Whether to keep pieces face-down.
     */
    void setup(int hidden = 1);

    /*
     * Check winner, pass a WinCon if you want to know how the game ended too
     * @param   wc  Ignored in HW1
     * @returns Red/Black   if all black/non-duck-red pieces have been eliminated
     *          NO_COLOR    if the above is not true
     */
    Color winner(WinCon *wc = nullptr) const;

    /*
     * @returns Red/Black   The color to play.
     */
    Color due_up() const { return sideToMove; }

    /*
     * Gets the time remaining.
     * Not available for HW1.
     *
     * @param   color   The color whose time to get.
     *                  Defaults to the side to play (due_up()).
     *
     * @returns The remaining time in seconds
     */
    double time_left(Color color = Mystery) const;

    /*
     * Get bitboards for chosen pieces.
     * A. Specify one or more PieceType's.
     *
     * @param   pts...  The type(s) of pieces requested
     *                  Defaults to ALL_PIECES
     *
     * @returns A bitboard of squares with the requested pieces
     * @note    Gives all pieces that are *ANY* of the request types.
     */
    Board pieces(PieceType pt = ALL_PIECES) const
    {
        assert(pt < PIECE_TYPE_NB);
        return byTypeBB[pt];
    }

    template<typename... PieceTypes>
    Board pieces(PieceType pt, PieceTypes... pts) const
    {
        return pieces(pt) | pieces(pts...);
    }

    /*
     * Get bitboards for chosen pieces.
     * B. Specify a color and one or more PieceType's.
     *
     * @param   c       The color of pieces requested
     * @param   pts...  The type(s) of pieces requested
     *                  Defaults to ALL_PIECES (in effect).
     *
     * @returns A bitboard of pieces with the requested pieces
     * @note    Gives pieces are of the request color *AND* *ANY* of the requested piece types.
     */
    Board pieces(Color c) const
    {
        assert(c < SIDE_NB);
        return byColorBB[c];
    }

    template<typename... PieceTypes>
    Board pieces(Color c, PieceTypes... pts) const
    {
        return pieces(c) & pieces(pts...);
    }

    /*
     * Counts the number of chosen pieces.
     * @param   c   Optional, the color of pieces requested.
     *              If left empty, counts both colors.
     * @param   pt  The type of pieces requested.
     *              Defaults to ALL_PIECES.
     *
     * @return  The number of requested pieces
     * @note    This function only accepts one piece type.
     */
    inline int count(Color c, PieceType pt = ALL_PIECES) const
    {
        assert(pt < PIECE_TYPE_NB);
        if (c == Red || c == Black) {
            return __builtin_popcount(byColorBB[c] & byTypeBB[pt]);
        }
        return __builtin_popcount(byTypeBB[pt]);
    }
    inline int count(PieceType pt = ALL_PIECES) const { return count(Mystery, pt); }

    /*
     * Gets a Board of all pieces that are
     *   - of lower or equal rank to _pt_
     *   - not of color _c_
     *
     * @param   c   Color of the "capturer"
     * @param   pt  Type of the "capturer"
     * @returns A bitboard as specified above
     * @note    This is a superset of legal captures as it ignores movement rules.
     */
    Board subordinates(Color c, PieceType pt) const;

    /*
     * Places a piece at a square. If a piece already exists, it will be replaced.
     * @param   p   The piece to place.
     * @param   sq  The square
     */
    void place_piece_at(const Piece &p, Square sq);

    /*
     * Removes a piece at a square.
     * @param   sq  The square
     * @returns The piece that is removed
     */
    Piece remove_piece_at(Square sq);

    /*
     * Removes and places the piece somewhere else.
     * Convenient wrapper for the two functions above.
     *
     * @param   src Source square
     * @param   dst Destination square
     */
    void move_piece(Square src, Square dst);

    /*
     * Gets a piece at a square. This is remove_piece_at() without the remove part.
     * @param   sq  The square
     * @returns The piece
     */
    Piece peek_piece_at(Square sq) const { return board[sq]; }

    /*
     * Flips a face-down piece.
     * @param   sq  The square
     * @returns Whether the flip was successful
     * @note    This is a non-deterministic operation.
     *          If you make multiple copies of this position and run this function
     *          on the same square on each, it may well yield different pieces.
     */
    bool flip_piece_at(Square sq);

    /*
     * Performs a move.
     * @param   mv  The move to perform
     * @return  Whether the move was successful
     */
    bool do_move(const Move &mv);

    /*
     * Undoes the last successful move. This may be called multiple times.
     *
     * The following are restored:
     *      - board state
     *      - hidden pieces pool
     *      - 50-move rule count
     *
     * The following are NOT restored:
     *      - times
     *
     * @return  True for success
     */
    bool undo_move();

    /*
     * Simulates a game until the end, playing moves using _strategy_.
     * The default random strategy is supplied as strategy_random() in helper.h
     *
     * @param   strategy    A function that selects a Move given a MoveList<>
     * @return  1 if the current side to move wins.
     *          0 if the simulation ends in a draw.
     *          -1 if the current side to move loses.
     */
    int simulate(Move (*strategy)(MoveList<> &moves));
};

std::ostream &operator<<(std::ostream &os, const Position &pos);

#endif