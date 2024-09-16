#pragma once

namespace potential {
  template <typename EnergyGame, typename PotentialTeller>
  class potential_firstmax : public potential_computer<EnergyGame, PotentialTeller> {
      using weight_t = EnergyGame::weight_t;
      using potential_computer<EnergyGame, PotentialTeller>::nrg_game;
      using potential_computer<EnergyGame, PotentialTeller>::teller;
      using potential_computer<EnergyGame, PotentialTeller>::potential;
      std::vector<vertex_t> strat;

    public:
      potential_firstmax (const EnergyGame& nrg_game, const PotentialTeller& teller, logger_t& logger, int trace) :
        potential_computer<EnergyGame, PotentialTeller> (nrg_game, teller, logger, trace),
        strat (nrg_game.size (), -1) {}

      std::optional<vertex_t> strategy_for (vertex_t v) {
        if (nrg_game.is_min (v) and
            teller.get_potential ()[v] < nrg_game.get_infty ()) {
          for (auto&& o : nrg_game.outs (v))
            if (teller.get_potential ()[o.second] < nrg_game.get_infty () and o.first <= 0)
              return o.second;
          assert (false);
        }
        if (nrg_game.is_max (v) and
            teller.get_potential ()[v] >= nrg_game.get_infty ())
          return -1; // Winning, but no clue about strategy.
        return std::nullopt; // Losing
      }

      void compute () {
        for (auto& p : potential)
          p = nrg_game.get_infty ();

        std::vector<vertex_t> max_backtrack;

        for (auto&& v : teller.undecided_vertices ()) {
          if (nrg_game.is_max (v) ) {
            potential[v] = nrg_game.outs (v)[0].first;
            for (auto& tr : nrg_game.outs (v) | std::views::drop (1)) {
              if (potential[v] < tr.first)
                potential[v] = tr.first;
            }
            if (teller.get_potential ()[v] + potential[v] >= nrg_game.get_infty ())
              max_backtrack.push_back (v);
          }
          else {
            potential[v] = nrg_game.outs (v)[0].first;
            for (auto& tr : nrg_game.outs (v) | std::views::drop (1)) {
              if (potential[v] > tr.first)
                potential[v] = tr.first;
            }
          }
          if (potential[v] < 0)
            potential[v] = zero_number (*nrg_game.get_infty ());
          if (teller.get_potential ()[v] + potential[v] >= nrg_game.get_infty ())
            max_backtrack.push_back (v);
        }
        while (not max_backtrack.empty ()) {
          auto v = max_backtrack.back ();
          max_backtrack.pop_back ();
          if (nrg_game.is_min (v)) { // Losing min; all its min successors are losing, all its max are wining.
            for (auto&& wi : nrg_game.ins (v)) {
              auto i = wi.second;
              if (teller.get_potential ()[i] + potential[i] < nrg_game.get_infty ()) {
                potential[i] = nrg_game.get_infty ();
                max_backtrack.push_back (i);
              }
            }
          }
          else { // Winning max, all its predecessor max are winning
            for (auto&& wi : nrg_game.ins (v)) {
              auto i = wi.second;
              if (nrg_game.is_max (i) and
                  teller.get_potential ()[i] + potential[i] < nrg_game.get_infty ()) {
                potential[i] = nrg_game.get_infty ();
                max_backtrack.push_back (i);
              }
            }
          }
        }
      }
  };
}
