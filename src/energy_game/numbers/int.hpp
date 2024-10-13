#pragma once

#include <cstdint>

class gmp;

template <>
inline
bool set_if_plus_larger<int64_t> (int64_t& w, const int64_t& w1, const int64_t& w2) {
  if (w < w1 + w2) {
    w = w1 + w2;
    return true;
  }
  return false;
}

template <>
inline
bool set_if_plus_smaller<int64_t> (int64_t& w, const int64_t& w1, const int64_t& w2) {
  if (w > w1 + w2) {
    w = w1 + w2;
    return true;
  }
  return false;
}

#ifndef NOINK
template <>
inline
int64_t priority_to_number<int64_t> (const priority_t& prio,
                                     const pg::Game& pgame,
                                     bool swap) {
#ifdef GAMES_ARE_NRG
  return static_cast<int64_t> (::priority_to_number<gmp> (prio, pgame, swap));
#else
  int64_t base = -1 * (int64_t) pgame.nodecount ();
  if (swap)
    return -(pow (base, prio));
  else
   return pow (base, prio);
#endif
}

template <>
inline
int64_t infinity_number<int64_t> (const pg::Game& pgame) {
#ifdef GAMES_ARE_NRG
  return static_cast<int64_t> (::infinity_number<gmp> (pgame));
#else
  return pow (pgame.nodecount (), abs (pgame.priority (pgame.nodecount () - 1)) + 1);
#endif
}

template <>
inline
int64_t zero_number<int64_t> (const int64_t&) {
  return 0;
}
#endif // NOINK
