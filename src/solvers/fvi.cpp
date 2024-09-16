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

#include "energy_game.hpp"
#include "solvers/potential/potential_teller.hpp"
#include "solvers/potential/potential_computers.hpp"

#ifdef NDEBUG
# define log(T)
#else
# define log(T) do { if (this->trace >= 1) { this->logger << T; } } while (0)
#endif

#define log_stat(T) do { std::cout << T; } while (0)

namespace pg {
  FVISolver::FVISolver (Oink& oink, Game& game) :
    Solver (oink, game),
    nrg_game (this->game, logger, trace)
  { }

  FVISolver::~FVISolver() { } // must be defined.

  void FVISolver::run() {
    using namespace std::chrono;
    potential::stats::eg_pot_update = 0;
    potential::stats::eg_reduce = 0;
    potential::stats::pot_compute = 0;
    potential::stats::pot_iter = 0;
    potential::stats::pot_phase2 = 0;
    potential::stats::pot_backtrack = 0;

    using namespace std::literals; // enables literal suffixes, e.g. 24h, 1ms, 1s.

    auto teller = potential::potential_teller (nrg_game);
    auto computer = potential::potential_fvi_alt (nrg_game, teller, logger, trace);

    auto time_sol_beg = high_resolution_clock::now();

    log ("Infinity: " << nrg_game.get_infty () << std::endl);
    do {
      log (nrg_game << std::endl);
      computer.compute ();
      log ("Potential: " << computer << std::endl);
    } while (teller.reduce (computer.get_potential ()));

    std::cout << "solving: "
              << duration_cast<nanoseconds>(high_resolution_clock::now () - time_sol_beg) / 1ms << "\n";

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
  }
}

#undef log_stat
#undef log
