#pragma once

namespace potential {
  template <typename EnergyGame, typename PotentialTeller>
  class potential_fvi_alt : public potential_computer<EnergyGame, PotentialTeller> {
    private:
      potential_fvi_swap<false, EnergyGame, PotentialTeller> fvi;
      potential_fvi_swap<true, EnergyGame, PotentialTeller> fvi_swap;
      bool swap = true;
    public:
      potential_fvi_alt (EnergyGame& game, PotentialTeller& teller, logger_t& logger, int trace) :
        potential_computer<EnergyGame, PotentialTeller> (game, teller, logger, trace),
        fvi (game, teller, logger, trace), fvi_swap (game, teller, logger, trace) {}


      std::optional<vertex_t> strategy_for (vertex_t v) {
        if (this->nrg_game.is_min (v))
          return fvi_swap.strategy_for (v);
        else
          return fvi.strategy_for (v);
      }

      void compute () {
        swap ^= true;
        if (swap)
          fvi_swap.compute ();
        else
          fvi.compute ();

        // If it's all zeroes, then do one more round of the other fvi and be done.
        auto&& pot = get_potential ();
        bool no_change = true;
        for (const auto& v : this->teller.undecided_vertices ())
          if (pot[v] != 0) {
            no_change = false;
            break;
          }
        if (not no_change)
          return;
        swap ^= true;
        if (swap)
          fvi_swap.compute ();
        else
          fvi.compute ();
      }

      virtual
      const potential_computer<EnergyGame, PotentialTeller>::potential_t& get_potential () const {
        if (swap)
          return fvi_swap.get_potential ();
        else
          return fvi.get_potential ();
      }
  };
}
