#pragma once

namespace potential {
  template <typename EnergyGame, typename PotentialTeller>
  class potential_computer {
    protected:
      using weight_t = typename EnergyGame::weight_t;
      using potential_t = std::vector<weight_t>;
      EnergyGame&              nrg_game;
      PotentialTeller&         teller;

      logger_t& logger;
      int trace = 0;
    public:
      potential_computer (EnergyGame& ngame,
                          PotentialTeller& teller,
                          logger_t& logger, int trace) :
        nrg_game (ngame), teller (teller), logger (logger), trace (trace) {
      }

      virtual void compute () = 0;
  };

  template <typename EG, typename PT>
  std::ostream& operator<< (std::ostream& os, const potential_computer<EG, PT>&) {
    return os;
  }
}

#include "solvers/potential/potential_computers/vi.hpp"
#include "solvers/potential/potential_computers/fvi.hpp"
#include "solvers/potential/potential_computers/fvi_phase1_pq.hpp"
#include "solvers/potential/potential_computers/fvi_qd.hpp"
#include "solvers/potential/potential_computers/fvi_nfvi.hpp"
#include "solvers/potential/potential_computers/fvi_alt.hpp"
