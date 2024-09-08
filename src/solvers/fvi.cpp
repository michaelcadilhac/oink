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

#include "solvers/fvi/weights.hpp"
#include "solvers/fvi/movable_number.hpp"

#include "solvers/fvi/potential_computers.hpp"
#include "solvers/fvi/stats.hpp"

#include "solvers/fvi/energy_game.hpp"


using int64_weight_t  = movable_number<int64_t>;
using gmp_weight_t    = movable_number<gmp>;
using vec_weight_t    = movable_number<ovec<int32_t>>;
using map_weight_t    = movable_number<omap<int32_t, int32_t>>;

using weight_t    = int64_weight_t;

namespace pg {
  FVISolver::FVISolver (Oink& oink, Game& game) : Solver (oink, game) { }

  FVISolver::~FVISolver() { } // must be defined.

  void FVISolver::run() {
    using namespace std::chrono;
    fvi_stats::eg_pot_update = 0;
    fvi_stats::eg_reduce = 0;
    fvi_stats::pot_compute = 0;
    fvi_stats::pot_iter = 0;
    fvi_stats::pot_phase2 = 0;
    fvi_stats::pot_backtrack = 0;

    using namespace std::literals; // enables literal suffixes, e.g. 24h, 1ms, 1s.

    auto time_conv_beg = high_resolution_clock::now();
    auto nrg_game = energy_game<weight_t> (this->game, logger, trace);
    auto time_sol_beg = high_resolution_clock::now();
    std::cout << "conversion: "
              << duration_cast<nanoseconds>(time_sol_beg - time_conv_beg) / 1ms << "\n";

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

    std::cout << "solving: "
              << duration_cast<nanoseconds>(high_resolution_clock::now () - time_sol_beg) / 1ms << "\n";

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
