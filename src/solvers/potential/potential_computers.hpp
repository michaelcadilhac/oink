#pragma once

namespace potential {
  template <typename EnergyGame, typename PotentialTeller>
  class potential_computer {
    protected:
      using weight_t = typename EnergyGame::weight_t;
      using potential_t = std::vector<weight_t>;
      const EnergyGame&        nrg_game;
      const PotentialTeller&   teller;
      potential_t              potential;

      logger_t& logger;
      int trace = 0;
    public:
      potential_computer (const EnergyGame& ngame,
                          const PotentialTeller& teller,
                          logger_t& logger, int trace) :
        nrg_game (ngame), teller (teller), logger (logger), trace (trace) {
        potential.reserve (nrg_game.size ());
        for (size_t i = 0; i < nrg_game.size (); ++i)
          potential.push_back (zero_number (*nrg_game.get_infty ()));
      }

      virtual const potential_t& get_potential () const {
        return potential;
      }

      virtual void compute () = 0;
  };

  template <typename EG, typename PT>
  std::ostream& operator<< (std::ostream& os, const potential_computer<EG, PT>& pc) {
    size_t i = 0;
    os << "[ ";
    for (auto&& elt : pc.get_potential ())
      os << i++ << "->" << elt << " ";
    os << "]";

    return os;
  }
}

#include "solvers/potential/potential_computers/vi.hpp"
#include "solvers/potential/potential_computers/fvi.hpp"
#include "solvers/potential/potential_computers/fvi_phase1_pq.hpp"
#include "solvers/potential/potential_computers/fvi_qd.hpp"
#include "solvers/potential/potential_computers/fvi_nfvi.hpp"
#include "solvers/potential/potential_computers/fvi_alt.hpp"
