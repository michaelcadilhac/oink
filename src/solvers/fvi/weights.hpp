#pragma once

#include <iostream>

#include "solvers/fvi/utils.hpp"

template <typename T>
bool set_if_plus_larger (T& w, const T& w1, const T& w2) {
  return w.set_if_plus_larger (w1, w2);
}

template <typename T>
bool set_if_plus_smaller (T& w, const T& w1, const T& w2) {
  return w.set_if_plus_smaller (w1, w2);
}

template <typename T>
T priority_to_weight (const priority_t& prio,
                      const pg::Game& pgame,
                      bool swap) {
  return T::priority_to_weight (prio, pgame, swap);
}

template <typename T>
T infinity_weight (const pg::Game& pgame) {
  return T::infinity_weight (pgame);
}

template <typename T>
T zero_weight (const T& t) {
  return T::zero_weight (t);
}

/**
 * All weights must define:
 */
template <typename W>
concept Weight =
  std::is_constructible_v<W, W&&> and
  std::is_constructible_v<W, const W&> and
  std::is_default_constructible_v<W> and
  requires (W& w, const W& cw, const pg::Game& pgame, std::ostream& os) {
    { set_if_plus_larger (w, cw, cw) } -> std::same_as<bool>;
    { set_if_plus_smaller (w, cw, cw) } -> std::same_as<bool>;
    w -= w;
    w += w;
    w == -w;
    cw < 0 == cw > 0;
    w = priority_to_weight<W> (0, pgame, true);
    w = infinity_weight<W> (pgame);
    w = zero_weight (w);
  };

namespace std {
  template <Weight W>
  inline
  std::ostream& operator<<(std::ostream& os, const W& w) { return w.print (os); }
}

#include "solvers/fvi/weights/omap.hpp"
#include "solvers/fvi/weights/gmp.hpp"
#include "solvers/fvi/weights/ovec.hpp"
#include "solvers/fvi/weights/int.hpp"
