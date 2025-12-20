// Chinese Dark Chess: types
// ----------------------------------

#ifndef TYPES_H
#define TYPES_H

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// -~ Colors ~-
enum Color { Black, Red, SIDE_NB, Mystery, NO_COLOR };

// -~ Squares ~-
// clang-format off
enum Square : int {
    SQ_A1, SQ_B1, SQ_C1, SQ_D1, SQ_E1, SQ_F1, SQ_G1, SQ_H1,
    SQ_A2, SQ_B2, SQ_C2, SQ_D2, SQ_E2, SQ_F2, SQ_G2, SQ_H2,
    SQ_A3, SQ_B3, SQ_C3, SQ_D3, SQ_E3, SQ_F3, SQ_G3, SQ_H3,
    SQ_A4, SQ_B4, SQ_C4, SQ_D4, SQ_E4, SQ_F4, SQ_G4, SQ_H4,
    /* Flipped vertically (compared to board output to stdout) */
    SQUARE_NB   = 32,
    SQ_NONE     = 42
};
// clang-format on

enum File : int { FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H, FILE_NB };
enum Rank : int { RANK_1, RANK_2, RANK_3, RANK_4, RANK_NB };

// -~ Directions ~-
enum Direction : int {
    NORTH = 8,
    EAST  = 1,
    SOUTH = -NORTH,
    WEST  = -EAST,

    NORTH_EAST = NORTH + EAST,
    SOUTH_EAST = SOUTH + EAST,
    SOUTH_WEST = SOUTH + WEST,
    NORTH_WEST = NORTH + WEST
};

// -~ Pieces ~-
enum PieceType {
    General = 0,
    Advisor,
    Elephant,
    Chariot,
    Horse,
    Cannon,
    Soldier,
    // Duck
    Duck,
    // Hidden type
    Hidden,
    // special type
    FACE_UP,
    ALL_PIECES,
    // counts
    PIECE_TYPE_NB,                                   // base + duck + hidden + special
    REAL_PIECE_TYPE_NB    = PIECE_TYPE_NB - 2,       // base + duck + hidden
    SHOWN_PIECE_TYPE_NB   = REAL_PIECE_TYPE_NB - 1,  // base + duck
    MOVABLE_PIECE_TYPE_NB = SHOWN_PIECE_TYPE_NB - 1, // base
    NO_PIECE              = 42
};

extern const char PIECE2CHAR[SIDE_NB][REAL_PIECE_TYPE_NB];
extern const std::string PIECE2WIDECHAR[SIDE_NB][REAL_PIECE_TYPE_NB];

struct Piece {
    Color side;
    PieceType type;

    /*
     * No piece.
     */
    Piece()
      : side(Color::NO_COLOR)
      , type(PieceType::NO_PIECE)
    {}

    /*
     * Yes piece.
     */
    Piece(Color side, PieceType type)
      : side(side)
      , type(type)
    {
        assert((side == Mystery) == (type == Hidden));
    }

    explicit operator std::string() const
    {
        if (side == Color::Mystery) {
            return "??";
        }
        if (side == Color::NO_COLOR) {
            return "  ";
        }
        return PIECE2WIDECHAR[side][type];
    }

    explicit operator char() const
    {
        if (side == Color::Mystery) {
            return '?';
        }
        if (side == Color::NO_COLOR) {
            return ' ';
        }
        return PIECE2CHAR[side][type];
    }
};

using Board = uint32_t;

struct BoardView {
    struct Iterator {
        Board b;
        Square s;

        Iterator(Board b)
          : b(b)
          , s(next())
        {}

        Square operator*() const { return s; }
        Iterator &operator++()
        {
            b &= b - 1;
            s = next();
            return *this;
        }
        bool operator!=(const Iterator &other) const { return b != other.b; }

        private:
        Square next() const { return static_cast<Square>(b ? __builtin_ctz(b) : 32); }
    };

    Board b;

    BoardView(Board b)
      : b(b)
    {}

    /*
     * Collects into a vector.
     * @param   self    The BoardView on a Board
     * @returns A vector of all set Squares of the Board
     */
    inline std::vector<Square> to_vector() const
    {
        std::vector<Square> v;
        for (Square sq : *this) {
            v.push_back(sq);
        }
        return v;
    }

    Iterator begin() const { return Iterator(b); }
    Iterator end() const { return Iterator(0); }
};

// -~ Moves ~-
// Stored as a number
// Bits 0 ~ 4: from
// Bits 5 ~ 9: to
// Bits 10 ~ 11: flags
//     - b01: move
//     - b10: flip
// Bits 12 ~ 15: unused
enum MoveType { Moving = 1, Flipping = 2, All = 3 };
class Move {
    private:
    uint16_t raw;

    public:
    Move() = default;
    constexpr explicit Move(uint16_t d)
      : raw(d)
    {}
    constexpr Move(Square from, Square to)
      : raw(from + (to << 5))
    {
        raw |= (from == to) ? (MoveType::Flipping << 10) : (MoveType::Moving << 10);
    }

    constexpr operator uint16_t() { return raw; }
    constexpr Move &operator=(const Move &other)
    {
        raw = other.raw;
        return *this;
    }
    bool operator<(const Move &other) const { return raw < other.raw; }
    bool operator>(const Move &other) const { return raw > other.raw; }
    bool operator<=(const Move &other) const { return raw <= other.raw; }
    bool operator>=(const Move &other) const { return raw >= other.raw; }
    bool operator==(const Move &other) const { return raw == other.raw; }
    bool operator!=(const Move &other) const { return raw != other.raw; }

    /*
     * Access various properties.
     */
    MoveType type() const { return MoveType((raw >> 10) & All); }
    Square from() const { return Square(raw & 0x1F); }
    Square to() const { return Square((raw >> 5) & 0x1F); }
};

// -~ WinCon ~-
// Win conditions
class WinCon {
    public:
    enum Value : uint8_t {
        // Wins
        Elimination,
        DeadPosition,
        IllegalMove,
        // Draws
        InsufficentMaterial,
        FiftyMoves,
        Threefold,
    };
    WinCon() = default;
    constexpr WinCon(Value wc)
      : value(wc)
    {}
    explicit operator bool() const = delete;

    constexpr bool operator==(WinCon a) const { return value == a.value; }
    constexpr bool operator!=(WinCon a) const { return value != a.value; }

    std::string to_string() const;

    private:
    Value value;
};

// -~ StateInfo ~-
// Records various stats about a position
struct StateInfo {
    int fiftyMoveCount;
    Color illegal;
    std::pair<double, double> time_remaining; // RED, BLACK
};

// -~ PastMoves ~-
// Records moves done, for unwinding
struct PastMove {
    Move mv;
    Piece p;        // Either the piece flipped, or the piece captured
    int fmc_old;    // save the fifty move counter
};

class Position;

#endif