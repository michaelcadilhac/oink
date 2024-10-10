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

#pragma once

#include "oink/solver.hpp"
#include "energy_game.hpp"
#include "solvers/potential/potential_teller.hpp"
#include "solvers/potential/potential_computers.hpp"

#include "solvers/potential/stats.hpp"

ADD_TIME_TO_STATS (compute);
ADD_TIME_TO_STATS (tm_solving);

#ifdef NDEBUG
# define log(T)
#else
# define log(T) do { if (this->trace >= 1) { this->logger << T; } } while (0)
#endif


namespace pg {
  template <template <typename EG, typename PT> typename PotentialComputer>
  class FVISolver : public Solver {
    public:
      FVISolver (Oink& oink, Game& game)  :
        Solver (oink, game),
        nrg_game (this->game, logger, trace),
        teller (nrg_game),
        computer (nrg_game, teller, logger, trace)
      { }

      virtual ~FVISolver () {}

      virtual void run () {
        CLEAR_STATS;

        using namespace std::literals; // enables literal suffixes, e.g. 24h, 1ms, 1s.

        START_TIME (tm_solving);

        log ("Infinity: " << nrg_game.get_infty () << std::endl);
        do {
          log (nrg_game << std::endl);
          START_TIME (compute);
          computer.compute ();
          STOP_TIME (compute);
          log ("Potential: " << computer << std::endl);
        } while (teller.reduce (computer.get_potential ()));

        STOP_TIME (tm_solving);

        log_stat ("solving: " << GET_TIME (tm_solving) << "\n");

        auto&& pot = teller.get_potential ();
        for (auto&& v : nrg_game.vertices ()) {
          if (game.isSolved (v)) continue;
          log ("vertex " << v << (nrg_game.is_max (v) ? " (max) " : " (min) "));
          log (" potential " << pot[v]);
          if (auto strat = computer.strategy_for (v)) {
            log (" take (" << v << ", " << *strat << ")\n");
            Solver::solve (v, not nrg_game.is_max (v), *strat);
          }
          else {
            log (" (losing)\n");
            Solver::solve (v, nrg_game.is_max (v), -1);
          }
        }

        log_stat ("stat: eg_pot_update = " << potential::stats::eg_pot_update << "\n");
        log_stat ("stat: eg_reduce = " << potential::stats::eg_reduce << "\n");
        log_stat ("stat: pot_compute = " << potential::stats::pot_compute << "\n");
        log_stat ("stat: pot_iter = " << potential::stats::pot_iter << "\n");
        log_stat ("stat: pot_phase2 = " << potential::stats::pot_phase2 << "\n");
        log_stat ("stat: pot_backtrack = " << potential::stats::pot_backtrack << "\n");

        log_stat ("stat: pot_sign = " << potential::stats::pot_sign << "\n");
        log_stat ("stat: pot_sign_recompute = " << potential::stats::pot_sign_recompute << "\n");


#define PRINT_TIME(Field) log_stat ("timestat: " #Field " = " << GET_TIME (Field) << "\n");

        PRINT_TIME (compute);
        PRINT_TIME (tm_teller_edge_search);
        PRINT_TIME (tm_reduce);
        PRINT_TIME (tm_reduce_update_pot);
        PRINT_TIME (tm_reduce_isolate);
        PRINT_TIME (tm_reduce_set_difference);
        PRINT_TIME (tm_reduce_update_edges);
      }
    private:
      using weight_t    = gmp_weight_t;
      // The energy game has two extra pieces of info for the potential teller.
      using energy_game_t = energy_game<weight_t, potential::extra_edge_info<weight_t>>;
      using teller_t = potential::potential_teller<energy_game_t>;

      energy_game_t nrg_game;
      teller_t teller;
      PotentialComputer<energy_game_t, teller_t> computer;
  };
}

#undef log_stat
#undef log
