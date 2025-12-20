// Wakasagihime
// Plays Chinese Dark Chess (Banqi)!

#include "lib/chess.h"
#include "lib/marisa.h"
#include "lib/types.h"
#include <sstream>

// Girls are preparing...
__attribute__((constructor)) void prepare()
{
    // Prepare the distance table
    for (Square i = SQ_A1; i < SQUARE_NB; i += 1) {
        for (Square j = SQ_A1; j < SQUARE_NB; j += 1) {
            SquareDistance[i][j] = distance<Rank>(i, j) + distance<File>(i, j);
        }
    }

    // Prepare the attack table (regular)
    Direction dirs[4] = { NORTH, SOUTH, EAST, WEST };
    for (Square sq = SQ_A1; is_okay(sq); sq += 1) {
        Board a = 0;
        for (Direction d : dirs) {
            a |= safe_destination(sq, d);
        }
        PseudoAttacks[sq] = a;
    }

    // Prepare magic
    init_magic<Cannon>(cannonTable, cannonMagics);
}

// le fishe
// HW2 Version - Wakasagi plays games at the Sushi Server's requests
int main()
{
    std::string line;
    std::string command;

    Position *pos = nullptr;
    Move mv;
    WinCon wc;

    while (std::getline(std::cin, line)) {
        std::istringstream iss(line);
        iss >> command;

        // START <hidden>
        if (command.compare("START") == 0) {
            int hidden;
            if (!(iss >> hidden)) {
                info << "ERR Bad Command\n";
                continue;
            }
            if (pos != nullptr) {
                delete pos;
            }
            pos = new Position();
            pos->add_collection();
            pos->setup(hidden);
            info << "OK\n";
            continue;
        }

        // STATE
        if (command.compare("STATE") == 0) {
            if (pos == nullptr) {
                info << "ERR Run START first\n";
                continue;
            }

            Color winner = pos->winner(&wc);
            if (winner == NO_COLOR) {
                info << "IN-PLAY\n" << pos->toFEN() << std::endl;
            } else if (winner == Red) {
                info << "RED WINS\n" << pos->toFEN() << "\n" << wc.to_string() << std::endl;
            } else if (winner == Black) {
                info << "BLACK WINS\n" << pos->toFEN() << "\n" << wc.to_string() << std::endl;
            } else if (winner == Mystery) {
                info << "DRAW\n" << pos->toFEN() << "\n" << wc.to_string() << std::endl;
            } else {
                info << "ERR Wakasagi hurt herself in confusion\n";
                return 1;
            }
            info << "OK\n";
            continue;
        }

        // MOVE <from> <to> / FLIP <square>
        if (command.compare("MOVE") == 0 || command.compare("FLIP") == 0) {
            if (pos == nullptr) {
                info << "ERR Run START first\n";
                continue;
            }

            std::istringstream iss_again(line);
            if (!(iss_again >> mv)) {
                info << "ERR Invalid Move Format\n";
                continue;
            }
            pos->do_move(mv);
            info << "OK\n";
            continue;
        }

        // QUIT
        if (command.compare("QUIT") == 0) {
            if (pos != nullptr) {
                delete pos;
            }
            info << "BYE\n";
            return 0;
        }

        info << "ERR Unknown command\n";
    }
}