/*
 * Copyright 2017-2018 Tom van Dijk, Johannes Kepler University Linz
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <algorithm>
#include <iomanip>
#include <ranges>

#include <list>
#include <set>
#include <queue>
#include <stack>

#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/gmp.hpp>

#include "oink/oink.hpp"

#include "solvers/fvi.hpp"

#include "solvers/fvi/energy_game.hpp"
#include "solvers/fvi/movable_number.hpp"
#include "solvers/fvi/potential_computers.hpp"
#include "solvers/fvi/stats.hpp"
#include "solvers/fvi/weights.hpp"


using gmp_weight_t    = lazy_number<boost::multiprecision::mpz_int>;
using vec_weight_t    = lazy_number<ovec<int32_t>>;
using map_weight_t    = lazy_number<omap<int32_t, int32_t>>;

using weight_t    = gmp_weight_t;

namespace pg {
  FVISolver::FVISolver (Oink& oink, Game& game) : Solver (oink, game) {
  }

  FVISolver::~FVISolver() {
    assert (lazy_maker<weight_t::number_t>::empty ());
  }

  void FVISolver::run() {
    using namespace std::chrono;
    fvi_stats::eg_pot_update = 0;
    fvi_stats::eg_reduce = 0;
    fvi_stats::pot_compute = 0;
    fvi_stats::pot_iter = 0;
    fvi_stats::pot_phase2 = 0;
    fvi_stats::pot_backtrack = 0;


    auto time_conv_beg = high_resolution_clock::now();
    auto nrg_game = energy_game<weight_t> (this->game, logger, trace);
    auto time_sol_beg = high_resolution_clock::now();
    std::cout << "conversion: " << duration_cast<duration<double>>(time_sol_beg - time_conv_beg).count() << "\n";

    auto potential_computer = potential_computers::potential_fvi_alt (nrg_game, logger, trace);

    log (nrg_game << std::endl);
    log ("Infinity: " << nrg_game.get_infty () << std::endl);
    while (not nrg_game.is_decided ()) {
      potential_computer.compute();
      log ("Potential: " << potential_computer << std::endl);
      auto&& new_potential = potential_computer.get_potential ();
      nrg_game.reduce (new_potential);
      log (nrg_game << std::endl);
    }

    std::cout << "solving: " << duration_cast<duration<double>>(high_resolution_clock::now () - time_sol_beg).count() << "\n";

    /*
     La stratégie gagnante pour Max (depuis les sommets qui sont détectés comme
     ayant potentiel infini, c'est à dire le complémentaire de F) c'est de
     prendre n'importe quelle arrête comme dans (1) : poids modifié non-negatif
     qui reste dans le complémentaire de F
     */
    // Send out the results

    auto&& pot = nrg_game.get_potential ();
    for (auto&& v : nrg_game.vertices ()) {
      if (game.isSolved (v)) continue;
      log ("vertex " << v << (nrg_game.is_max (v) ? " (max) " : " (min) "));
      log (" potential " << pot[v]);
      if (auto strat = potential_computer.strategy_for (v)) {
        log (" take (" << v << ", " << *strat << ")\n");
        Solver::solve (v, not nrg_game.is_max (v), *strat);
      }
      else {
        log (" (losing)\n");
        Solver::solve (v, nrg_game.is_max (v), -1);
      }
    }

    log_stat ("stat: eg_pot_update = " << fvi_stats::eg_pot_update << "\n");
    log_stat ("stat: eg_reduce = " << fvi_stats::eg_reduce << "\n");
    log_stat ("stat: pot_compute = " << fvi_stats::pot_compute << "\n");
    log_stat ("stat: pot_iter = " << fvi_stats::pot_iter << "\n");
    log_stat ("stat: pot_phase2 = " << fvi_stats::pot_phase2 << "\n");
    log_stat ("stat: pot_backtrack = " << fvi_stats::pot_backtrack << "\n");
    log ("solved" << std::endl);
  }
}
