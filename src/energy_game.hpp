#pragma once

#include "energy_game/energy_game.hpp"
#include "energy_game/movable_number.hpp"
#include "energy_game/numbers.hpp"

using int64_weight_t  = movable_number<int64_t>;
using gmp_weight_t    = movable_number<gmp>;
using vec_weight_t    = movable_number<ovec<int32_t>>;
using map_weight_t    = movable_number<omap<int32_t, int32_t>>;
