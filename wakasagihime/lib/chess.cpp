// Chinese Dark Chess
// ----------------------------------
// Kind of the main thing

#include "chess.h"
#include "cdc.h"
#include "marisa.h"
#include "types.h"
#include <cstddef>
#include <ostream>
#include <string>
#include <unordered_map>

const char PIECE2CHAR[SIDE_NB][REAL_PIECE_TYPE_NB] = {
    { 'K', 'A', 'E', 'R', 'N', 'C', 'P', 'D', '?' },
    { 'k', 'a', 'e', 'r', 'n', 'c', 'p', 'd', '?' }
};

const std::string PIECE2WIDECHAR[SIDE_NB][REAL_PIECE_TYPE_NB] = {
#if CHINESE_ENABLED
    { "將", "士", "象", "車", "馬", "包", "卒", "🦆", "??" },
    { "帥", "仕", "相", "俥", "傌", "炮", "兵", "🦆", "??" }
#else
    { "Ｋ", "Ａ", "Ｅ", "Ｒ", "Ｎ", "Ｃ", "Ｐ", "🦆", "??" },
    { "ｋ", "ａ", "ｅ", "ｒ", "ｎ", "ｃ", "ｐ", "🦆", "??" }
#endif
};

const std::unordered_map<char, Piece> CHAR2PIECE = {
    { '?', Piece(Mystery,   Hidden) },
    { 'K',   Piece(Black,  General) },
    { 'k',     Piece(Red,  General) },
    { 'A',   Piece(Black,  Advisor) },
    { 'a',     Piece(Red,  Advisor) },
    { 'E',   Piece(Black, Elephant) },
    { 'e',     Piece(Red, Elephant) },
    { 'R',   Piece(Black,  Chariot) },
    { 'r',     Piece(Red,  Chariot) },
    { 'N',   Piece(Black,    Horse) },
    { 'n',     Piece(Red,    Horse) },
    { 'C',   Piece(Black,   Cannon) },
    { 'c',     Piece(Red,   Cannon) },
    { 'P',   Piece(Black,  Soldier) },
    { 'p',     Piece(Red,  Soldier) },
    { 'D',   Piece(Black,     Duck) },
    { 'd',     Piece(Red,     Duck) },
};

uint8_t SquareDistance[SQUARE_NB][SQUARE_NB];

Board PseudoAttacks[SQUARE_NB];

std::ostream &operator<<(std::ostream &os, const Square &sq)
{
    os << (char)('A' + file_of(sq)) << (1 + rank_of(sq));
    return os;
}
std::istream &operator>>(std::istream &is, Square &sq)
{
    std::string txt;
    is >> txt;
    if (txt.size() != 2) {
        is.setstate(std::ios::failbit);
    } else {
        char &file = txt[0];
        char &rank = txt[1];
        sq         = static_cast<Square>((rank - '1') * 8 + (file - 'A'));
    }
    return is;
}

// Boards
// Returns an ASCII representation of a bitboard suitable
// to be printed to standard output. Useful for debugging.
std::string pretty(Board &b)
{

    std::string s = "+---+---+---+---+---+---+---+---+\n";

    for (Rank r = RANK_4; r >= RANK_1; r -= 1) {
        for (File f = FILE_A; f <= FILE_H; f += 1) {
            s += b & make_square(f, r) ? "| X " : "|   ";
        }
        s += "| " + std::to_string(1 + r) + "\n+---+---+---+---+---+---+---+---+\n";
    }
    s += "  a   b   c   d   e   f   g   h\n";

    return s;
}

template<PieceType pt>
inline Board attacks_bb(Square sq, Board occupied)
{
    switch (pt) {
        case Cannon:
            return cannonMagics[sq].attacks_bb(occupied);
        default:
            return PseudoAttacks[sq];
    }
}
template Board attacks_bb<General>(Square sq, Board occupied);
template Board attacks_bb<Advisor>(Square sq, Board occupied);
template Board attacks_bb<Elephant>(Square sq, Board occupied);
template Board attacks_bb<Chariot>(Square sq, Board occupied);
template Board attacks_bb<Horse>(Square sq, Board occupied);
template Board attacks_bb<Cannon>(Square sq, Board occupied);
template Board attacks_bb<Soldier>(Square sq, Board occupied);

// Variable version
Board attacks_bb(PieceType pt, Square sq, Board occupied)
{
    switch (pt) {
        case General:
            return attacks_bb<General>(sq, occupied);
        case Advisor:
            return attacks_bb<Advisor>(sq, occupied);
        case Elephant:
            return attacks_bb<Elephant>(sq, occupied);
        case Chariot:
            return attacks_bb<Chariot>(sq, occupied);
        case Horse:
            return attacks_bb<Horse>(sq, occupied);
        case Cannon:
            return attacks_bb<Cannon>(sq, occupied);
        case Soldier:
            return attacks_bb<Soldier>(sq, occupied);
        default:
            return 0;
    }
}

std::ostream &operator<<(std::ostream &os, const Move &mv)
{
    if (mv.type() == Flipping) {
        os << "FLIP " << mv.from() << std::endl;
    } else {
        os << "MOVE " << mv.from() << " " << mv.to() << std::endl;
    }
    return os;
}

std::istream &operator>>(std::istream &is, Move &mv)
{
    std::string cmd;
    is >> cmd;

    if (cmd == "FLIP") {
        Square sq;
        is >> sq;
        mv = Move(sq, sq);
    } else if (cmd == "MOVE") {
        Square from, to;
        is >> from >> to;
        mv = Move(from, to);
    } else {
        is.setstate(std::ios::failbit);
    }
    return is;
}

std::string WinCon::to_string() const
{
    switch (value) {
        case Elimination:
            return std::string("All pieces eliminated.");
        case DeadPosition:
            return std::string("Stalemate.");
        case IllegalMove:
            return std::string("Illegal move.");
        case FiftyMoves:
            return std::string("30 moves without captures.");
        case InsufficentMaterial:
            return std::string("Insufficient material.");
        case Threefold:
            return std::string("Threefold repetition.");
    }
    return std::string();
}

// Positions
void Position::clear()
{
    memset(byTypeBB, 0, sizeof(byTypeBB));
    memset(byColorBB, 0, sizeof(byColorBB));
    for (Square sq = SQ_A1; sq < SQUARE_NB; sq += 1) {
        board[sq] = Piece();
    }

    info.fiftyMoveCount = 0;
    info.illegal        = NO_COLOR;
    info.time_remaining = std::pair(0.0, 0.0);
}

Board Position::subordinates(Color c, PieceType pt) const
{
    Board b = 0;
    for (PieceType target = General; target < SHOWN_PIECE_TYPE_NB; target += 1) {
        if (pt > target) {
            b |= pieces(~c, target);
        }
    }
    return b;
}

void Position::place_piece_at(const Piece &p, Square sq)
{
    if (peek_piece_at(sq).side != NO_COLOR) {
        remove_piece_at(sq);
    }

    board[sq] = p;

    byTypeBB[p.type] |= sq;
    byTypeBB[ALL_PIECES] |= sq;

    if (p.side < SIDE_NB) {
        // is red or black (face up)
        byTypeBB[FACE_UP] |= sq;
        byColorBB[p.side] |= sq;
    }
}

Piece Position::remove_piece_at(Square sq)
{
    Piece p   = board[sq];
    board[sq] = Piece();

    byTypeBB[p.type] ^= sq;
    byTypeBB[ALL_PIECES] ^= sq;

    if (p.side < SIDE_NB) {
        byTypeBB[FACE_UP] ^= sq;
        byColorBB[p.side] ^= sq;
    }

    return p;
}

void Position::move_piece(Square src, Square dst)
{
    Piece removed = remove_piece_at(src);
    if (removed.type == NO_PIECE) {
        error << "move_piece: [Warn] Moved nothing." << std::endl;
        return;
    }
    place_piece_at(removed, dst);
}

bool Position::flip_piece_at(Square sq)
{
    if (peek_piece_at(sq).side != Mystery) {
        return false;
    }
    Piece new_piece =
        pieceCollection.empty() ? random_faceup_piece() : sample_remove(pieceCollection);

    place_piece_at(new_piece, sq);
    return true;
}

void Position::add_collection(Piece *set, size_t n)
{
    if (set == nullptr) {
        // use default piece set
        for (Color s : { Color::Red, Color::Black }) {
            // clang-format off
            pieceCollection
                << Piece(s, General)
                << Piece(s, Advisor)  << Piece(s, Advisor)
                << Piece(s, Elephant) << Piece(s, Elephant)
                << Piece(s, Chariot)  << Piece(s, Chariot)
                << Piece(s, Horse)    << Piece(s, Horse)
                << Piece(s, Cannon)   << Piece(s, Cannon)
                << Piece(s, Soldier)  << Piece(s, Soldier)
                << Piece(s, Soldier)  << Piece(s, Soldier)
                << Piece(s, Soldier);
            // clang-format on
        }
        return;
    }

    // Use provided n pieces
    for (int i = 0; i < n; i += 1) {
        pieceCollection << set[i];
    }
}

void Position::setup(int hidden)
{
    for (Square sq = SQ_A1; sq < SQUARE_NB; sq += 1) {
        place_piece_at(Piece(Mystery, Hidden), sq);
    }
    if (!hidden) {
        // flip all the pieces
        for (Square sq = SQ_A1; sq < SQUARE_NB; sq += 1) {
            flip_piece_at(sq);
        }
    }
}

void Position::readFEN(std::string fen)
{
    auto tokens = split_fen(fen);
    int i       = 0;
    Square sq   = SQ_A1;
    for (auto token : tokens) {
        switch (i) {
            case 0 ... 3:
                // parse a rank
                for (char &c : token) {
                    if (CHAR2PIECE.find(c) == CHAR2PIECE.end()) {
                        int empty_count = c - '0';
                        if (empty_count < 1 || empty_count > 8) {
                            error << "Warning: invalid FEN. \"" << c << "\"\n";
                            return;
                        }
                        sq += empty_count;
                    } else {
                        place_piece_at(CHAR2PIECE.at(c), sq);
                        sq += 1;
                    }
                }
                break;
            case 4:
                sideToMove = (token.compare("b") == 0) ? Black : Red;
                break;
            case 5:
                try {
                    info.time_remaining.first = std::stod(token);
                } catch (...) {
                    error << "Failed to parse first time!\n";
                }
                break;
            case 6:
                try {
                    info.time_remaining.second = std::stod(token);
                } catch (...) {
                    error << "Failed to parse second time!\n";
                }
                break;
        }
        i += 1;
    }
}

std::string Position::toFEN()
{
    short empty_count = 0;
    std::string fen;
    for (Square sq = SQ_A1; sq < SQUARE_NB; sq += 1) {
        Piece p = peek_piece_at(sq);
        if (p.type == NO_PIECE) {
            empty_count += 1;
        }
        if (p.type != NO_PIECE || file_of(sq) == FILE_H) {
            if (empty_count > 0) {
                fen += std::to_string(empty_count);
            }
            empty_count = 0;
            if (p.type != NO_PIECE) {
                fen += (char)p;
            }
        }
        if (sq == SQ_H1 || sq == SQ_H2 || sq == SQ_H3) {
            fen += '/';
        }
    }
    fen += (due_up() == Black) ? " b" : " r";
    return fen;
}

bool Position::do_move(const Move &mv)
{
    bool success = false;

    // == Flip ==
    if (mv.type() == Flipping) {
        Square sq = mv.from();
        if ((success = flip_piece_at(sq))) {
            /*
             * @note This is relevant for HW3 only.
             *
             * You are always assigned a side (Red/Black) at the start of a game.
             * In dark chess games, the first player to go is assigned "Red."
             * If that player then flips a Black piece, they become Black for the rest of the game.
             *
             */
            bool first_flip    = (__builtin_popcount(pieces(Hidden)) == 31);
            bool flipped_black = (pieces(Black) & mv.from());
            if (!first_flip || !flipped_black) {
                sideToMove = ~sideToMove;
            }

            // record flip
            history.push(PastMove {
                .mv      = mv,
                .p       = peek_piece_at(sq),
                .fmc_old = info.fiftyMoveCount,
            });
        }
        return success;
    }

    // == Move ==
    Square from = mv.from();
    Square to   = mv.to();

    const Piece src = peek_piece_at(from);
    const Piece dst = peek_piece_at(to);

    // Validate move
    if (src.side != sideToMove) {
        // Wrong side
        info.illegal = sideToMove;
        return false;
    }

    if (static_cast<int>(src.type) > static_cast<int>(Soldier)) {
        // Moving the wrong pieces
        info.illegal = sideToMove;
        return false;
    }

    Board valid_dest = attacks_bb(src.type, from, byTypeBB[ALL_PIECES]);
    if (!(valid_dest & to) || !(src.type > dst.type)) {
        // Not a valid move destination
        info.illegal = sideToMove;
        return false;
    }

    move_piece(from, to);

    // record move
    history.push(PastMove {
        .mv      = mv,
        .p       = dst,
        .fmc_old = info.fiftyMoveCount,
    });

    sideToMove = ~sideToMove;
    info.fiftyMoveCount = (dst.type == NO_PIECE) ? info.fiftyMoveCount + 1 : 0;
    return true;
}

bool Position::undo_move()
{
    if (history.size() == 0) {
        return false;
    }

    PastMove pmv = history.top();
    // restore board state
    switch (pmv.mv.type()) {
        case Moving:
        move_piece(pmv.mv.to(), pmv.mv.from()); // move back
        info.fiftyMoveCount = pmv.fmc_old;      // restore count
        if (pmv.p.type != NO_PIECE) {           // restore captured piece
            place_piece_at(pmv.p, pmv.mv.to());
        }
        break;

        case Flipping:
        if (pmv.p.type == NO_PIECE) {
            error << "undo_move - [Error] Flipped has type NO" << std::endl;
            return false;
        }
        place_piece_at(Piece(Mystery, Hidden), pmv.mv.from());
        info.fiftyMoveCount = pmv.fmc_old;
        pieceCollection << pmv.p;
        break;

        default:
        error << "undo_move - [Error] Move has type ALL" << std::endl;
        return false;
    }

    history.pop();
    sideToMove = ~sideToMove; // kurwa zapomnialem
    return true;
}

double Position::time_left(Color color) const
{
    if (color != Red && color != Black) {
        color = due_up();
    }
    switch (color) {
        case Red:
            return info.time_remaining.first;
        case Black:
            return info.time_remaining.second;
        default:
            return 0.0;
    }
}

Color Position::winner(WinCon *wc) const
{
    // 50-20 moves without captures
    if (info.fiftyMoveCount >= 30) {
        if (wc) {
            *wc = WinCon::FiftyMoves;
        }
        return Mystery;
    }

    // No legal moves
    MoveList<All, Black> moves_b(*this);
    if (moves_b.size() == 0) {
        if (wc) {
            *wc = count(Black) > 0 ? WinCon::DeadPosition : WinCon::Elimination;
        }
        return Red;
    }

    MoveList<All, Red> moves_r(*this);
    if (moves_r.size() == 0) {
        if (wc) {
            *wc = count(Red) > 0 ? WinCon::DeadPosition : WinCon::Elimination;
        }
        return Black;
    }

    // Insufficient material

    // Illegal moves
    if (info.illegal != NO_COLOR) {
        if (wc) {
            *wc = WinCon::IllegalMove;
        }
        return (info.illegal == Red) ? Black : Red;
    }

    // Keep playing
    return NO_COLOR;
}

int Position::simulate(Move (*strategy)(MoveList<> &moves))
{
    Position copy(*this);
    while (copy.winner() == NO_COLOR) {
        MoveList moves(copy);
        copy.do_move(strategy(moves));
    }
    if (copy.winner() == due_up()) {
        return 1;
    } else if (copy.winner() == Mystery) {
        return 0;
    }
    return -1;
}

// Print out your position easily
#define TEXT_RED "\033[31m"
#define TEXT_RST "\033[0m"
std::ostream &operator<<(std::ostream &os, const Position &pos)
{
    os << "\n +----+----+----+----+----+----+----+----+\n";

    for (Rank r = RANK_4; r >= RANK_1; r -= 1) {
        for (File f = FILE_A; f <= FILE_H; f += 1) {
            Piece p = pos.peek_piece_at(make_square(f, r));
            if (p.side == Red) {
                os << " | " << TEXT_RED << (std::string)p << TEXT_RST;
            } else {
                os << " | " << (std::string)p;
            }
        }
        os << " | " << (1 + r) << "\n +----+----+----+----+----+----+----+----+\n";
    }
    os << "    a    b    c    d    e    f    g    h\n"
       << (pos.due_up() == Black ? "-> Black" : "-> Red") << " to play\n";

    return os;
}
