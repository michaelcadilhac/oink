#pragma once

#include <iostream>


#include "energy_game/types.hpp"

#include "energy_game/numbers/gmp.hpp"
#include "energy_game/numbers/int.hpp"
#include "energy_game/numbers/omap.hpp"
#include "energy_game/numbers/ovec.hpp"

/// Default definitions:
template <typename T>
bool set_if_plus_larger (T& w, const T& w1, const T& w2) {
  return w.set_if_plus_larger (w1, w2);
}

template <typename T>
bool set_if_plus_smaller (T& w, const T& w1, const T& w2) {
  return w.set_if_plus_smaller (w1, w2);
}

template <typename T>
T priority_to_number (const priority_t& prio,
                      const pg::Game& pgame,
                      bool swap) {
  return T::priority_to_number (prio, pgame, swap);
}

template <typename T>
T infinity_number (const pg::Game& pgame) {
  return T::infinity_number (pgame);
}

template <typename T>
T zero_number (const T& t) {
  return T::zero_number (t);
}

namespace std {
  template <Number W>
  inline
  std::ostream& operator<<(std::ostream& os, const W& w) { return w.print (os); }
}
