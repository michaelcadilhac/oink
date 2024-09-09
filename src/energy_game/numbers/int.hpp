#pragma once

#include <cstdint>

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

template <>
inline
int64_t priority_to_number<int64_t> (const priority_t& prio,
                                     const pg::Game& pgame,
                                     bool swap) {
  int64_t base = -1 * (int64_t) pgame.nodecount ();
  if (swap)
    return -(pow (base, prio));
  else
    return pow (base, prio);
}

template <>
inline
int64_t infinity_number<int64_t> (const pg::Game& pgame) {
  return pow (pgame.nodecount (), pgame.priority (pgame.nodecount () - 1) + 1);
}

template <>
inline
int64_t zero_number<int64_t> (const int64_t&) {
  return 0;
}
