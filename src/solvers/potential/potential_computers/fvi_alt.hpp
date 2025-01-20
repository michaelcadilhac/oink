#pragma once

namespace potential {
  template <template <bool B, typename EG, typename PT> typename SwapComputer,
            typename EnergyGame, typename PotentialTeller>
  class potential_fvi_alt_gen : public potential_computer<EnergyGame, PotentialTeller> {
    private:
      SwapComputer<false, EnergyGame, PotentialTeller> computer;
      SwapComputer<true, EnergyGame, PotentialTeller>  computer_swap;
      bool swap = true;
    public:
      potential_fvi_alt_gen (EnergyGame& game, PotentialTeller& teller, logger_t& logger, int trace) :
        potential_computer<EnergyGame, PotentialTeller> (game, teller, logger, trace),
        computer (game, teller, logger, trace), computer_swap (game, teller, logger, trace) {}


      std::optional<vertex_t> strategy_for (vertex_t v) {
        if (this->nrg_game.is_min (v))
          return computer_swap.strategy_for (v);
        else
          return computer.strategy_for (v);
      }

      bool compute () {
        bool change;
        swap ^= true;
        if (swap)
          change = computer_swap.compute ();
        else
          change = computer.compute ();

        if (change)
          return true;

        // Do one more round of the other fvi and be done.
        swap ^= true;
        if (swap)
          computer_swap.compute ();
        else
          computer.compute ();

        return false; // We're done
      }
  };

  template <typename EnergyGame, typename PotentialTeller>
  using potential_fvi_alt = potential_fvi_alt_gen<potential::potential_fvi_swap,
                                                  EnergyGame, PotentialTeller>;

  template <typename EnergyGame, typename PotentialTeller>
  using potential_fvi_nfvi_alt = potential_fvi_alt_gen<potential::potential_fvi_nfvi_swap,
                                                       EnergyGame, PotentialTeller>;
}
