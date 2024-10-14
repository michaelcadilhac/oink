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
      potential_fvi_alt_gen (EnergyGame& game, PotentialTeller& teller) :
        potential_computer<EnergyGame, PotentialTeller> (game, teller),
        computer (game, teller), computer_swap (game, teller) {}


      std::optional<vertex_t> strategy_for (vertex_t v) {
        if (this->nrg_game.is_min (v))
          return computer_swap.strategy_for (v);
        else
          return computer.strategy_for (v);
      }

      void compute () {
        swap ^= true;
        if (swap)
          computer_swap.compute ();
        else
          computer.compute ();

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
          computer_swap.compute ();
        else
          computer.compute ();
      }

      virtual
      const potential_computer<EnergyGame, PotentialTeller>::potential_t& get_potential () const {
        if (swap)
          return computer_swap.get_potential ();
        else
          return computer.get_potential ();
      }
  };

  template <typename EnergyGame, typename PotentialTeller>
  using potential_fvi_alt = potential_fvi_alt_gen<potential::potential_fvi_swap,
                                                  EnergyGame, PotentialTeller>;

  template <typename EnergyGame, typename PotentialTeller>
  using potential_fvi_nfvi_alt = potential_fvi_alt_gen<potential::potential_fvi_nfvi_swap,
                                                       EnergyGame, PotentialTeller>;
}
