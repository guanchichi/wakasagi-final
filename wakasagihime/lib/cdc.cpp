// Chinese Dark Chess: utilities
// ----------------------------------

#include "cdc.h"

std::ostream &info  = std::cout;
std::ostream &error = std::cerr;
std::ostream &debug = std::cerr;

pcg32 rng(pcg_extras::seed_seq_from<std::random_device>{});

std::vector<std::string> split_fen(const std::string &fen)
{
    std::regex re("[/\\s]+"); // split on / or whitespace
    std::sregex_token_iterator it(fen.begin(), fen.end(), re, -1);
    return { it, {} };
}