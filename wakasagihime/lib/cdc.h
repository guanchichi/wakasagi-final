// Chinese Dark Chess: utilities
// ----------------------------------

#ifndef CDC_H
#define CDC_H

#include "pcg-cpp-0.98/include/pcg_random.hpp"
#include <iostream>
#include <random>
#include <regex>
#include <utility>

/*
 * Output streams for your convenience.
 * @global
 */
extern std::ostream &info;  // **Answers go here!**
extern std::ostream &error; // This is stderr
extern std::ostream &debug; // This is also stderr

/*
 * Pseudo-random number generator provided by PCG.
 * @global
 */
extern pcg32 rng;

/*
 * Vector syntax sugar so we can do `vec << stuff, more_stuff`
 */
template<typename T>
std::vector<T> &operator<<(std::vector<T> &v, const T &item)
{
    v.push_back(item);
    return v;
}
template<typename T>
std::vector<T> &operator,(std::vector<T> &v, const T &item)
{
    v.push_back(item);
    return v;
}

/*
 * Fast random sampling from vectors
 */
template<typename T>
T pop_at(std::vector<T> &v, typename std::vector<T>::size_type n)
{
    T thing = std::move_if_noexcept(v[n]);
    v[n]    = std::move_if_noexcept(v.back());
    v.pop_back();
    return thing;
}

template<typename T>
T sample_remove(std::vector<T> &v)
{
    return pop_at(v, rng(v.size()));
}

/*
 * @internal
 */
std::vector<std::string> split_fen(const std::string &fen);

#endif