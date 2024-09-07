#pragma once

#include <iostream>

#include "solvers/fvi/utils.hpp"

/**
 * All weights must define:
 */
template <typename W>
concept Weight =
  std::is_constructible_v<W, W&&> and
  std::is_constructible_v<W, const W&> and
  std::is_default_constructible_v<W> and
  requires (W& w, const W& cw, const pg::Game& pgame, std::ostream& os) {
    { w.set_if_plus_larger (w, w) } -> std::same_as<bool>;
    { w.set_if_plus_smaller (w, w) } -> std::same_as<bool>;
    w -= w;
    w += w;
    w == -w;
    cw < 0 == cw > 0;
    w = W::priority_to_weight (0, pgame, true);
    w = W::infinity_weight (pgame);
    w = W::zero_weight (w);
    { cw.print (os) } -> std::same_as<std::ostream&>;
  };

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

#include "solvers/fvi/weights/omap.hpp"
#include "solvers/fvi/weights/gmp.hpp"
#include "solvers/fvi/weights/ovec.hpp"

namespace std {
  template <Weight W>
  inline
  std::ostream& operator<<(std::ostream& os, const W& w) { return w.print (os); }
}
