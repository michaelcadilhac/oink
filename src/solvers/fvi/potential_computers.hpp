#pragma once

#include "solvers/fvi/energy_game.hpp"

namespace potential_computers {
  template <typename W>
  class potential_computer {
    protected:
      using potential_t = std::vector<W>;
      const energy_game<W>&        game;
      potential_t                  potential;

      logger_t& logger;
      int trace = 0;
    public:
      potential_computer (const energy_game<W>& game, logger_t& logger, int trace) :
        game (game), logger (logger), trace (trace) {
        potential.reserve (game.size ());
        for (size_t i = 0; i < game.size (); ++i)
          potential.push_back (zero_weight (*game.get_infty ()));
      }

      virtual const potential_t& get_potential () const {
        return potential;
      }

      virtual void compute () = 0;
  };

  template <typename W>
  std::ostream& operator<< (std::ostream& os, const potential_computer<W>& pc) {
    size_t i = 0;
    os << "[ ";
    for (auto&& elt : pc.get_potential ())
      os << i++ << "->" << elt << " ";
    os << "]";

    return os;
  }
}

#include "solvers/fvi/potential_computers/vi.hpp"
#include "solvers/fvi/potential_computers/fvi.hpp"
