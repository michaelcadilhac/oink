#pragma once

/**
 * All weights must define:

template <typename T>
bool set_if_plus_larger (T&, const T&, const T&);

template <typename T>
T priority_to_weight (const priority_t& prio,
                      const pg::Game& pgame,
                      bool swap);

template <typename T>
T infinity_weight (const pg::Game& pgame);

template <typename T>
T zero_weight (const T& t);
 */

#include "solvers/fvi/weights/omap.hpp"
#include "solvers/fvi/weights/gmp.hpp"
#include "solvers/fvi/weights/ovec.hpp"
