#pragma once

#include "solvers/stats.hpp"

ADD_TO_STATS (eg_reduce);
ADD_TO_STATS (eg_pot_update);

ADD_TIME_TO_STATS (tm_reduce);
ADD_TIME_TO_STATS (tm_reduce_update_pot);
ADD_TIME_TO_STATS (tm_reduce_isolate);
ADD_TIME_TO_STATS (tm_reduce_set_difference);
ADD_TIME_TO_STATS (tm_reduce_update_edges);

namespace potential {
  template <typename EnergyGame>
  class potential_teller {
      using weight_t = typename EnergyGame::weight_t;
      using potential_t = std::vector<weight_t>;

      EnergyGame& nrg_game;
      const weight_t& infty, minus_infty;
      std::vector<bool>  decided;

      potential_t potential;
      std::set<vertex_t> undecided_verts, undecided_max_verts, undecided_min_verts;

    public:
      potential_teller (EnergyGame& ngame) : nrg_game { ngame },
                                             infty { ngame.get_infty () },
                                             minus_infty { ngame.get_minus_infty () },
                                             decided (ngame.size ()) {
        potential.reserve (nrg_game.size ());
        for (size_t i = 0; i < nrg_game.size (); ++i)
          potential.push_back (zero_number<typename weight_t::number_t> (*infty));

        undecided_verts = std::set (nrg_game.vertices ().begin (), nrg_game.vertices ().end ());
        // TODO: some nodes are already solved: flush them by backward propagation.
      }

      const auto& undecided_vertices () const {
        return undecided_verts;
      }

      std::set<vertex_t> newly_decided;

      bool reduce (const potential_t& norm_pot) {
        TICK (eg_reduce);
        START_TIME (tm_reduce_update_pot);
        START_TIME (tm_reduce);

        bool changed = false;

        newly_decided.clear ();

        for (auto&& v : undecided_verts) {
          if (not decided[v] and norm_pot[v] != 0) {
            TICK (eg_pot_update);
            changed = true;
            if (norm_pot[v] >= infty)
              potential[v] = infty;
            else if (norm_pot[v] <= minus_infty)
              potential[v] = minus_infty;
            else
              potential[v] += norm_pot[v];
            if (potential[v] >= infty or potential[v] <= minus_infty) {
              newly_decided.insert (v);
              decided[v] = true;
            }
          }
        }
        STOP_TIME (tm_reduce_update_pot);
        if (not changed) {
          STOP_TIME (tm_reduce);
          // No need to update, we're done.
          return changed;
        }

        START_TIME (tm_reduce_isolate);
        // Remove decided nodes
        for (auto v : newly_decided)
          nrg_game.isolate_vertex (v);
        STOP_TIME (tm_reduce_isolate);

        START_TIME (tm_reduce_set_difference);
        std::set<vertex_t> result;
        std::set_difference (undecided_verts.begin(), undecided_verts.end(),
                             newly_decided.begin(), newly_decided.end(),
                             std::inserter (result, result.end()));
        std::swap (result, undecided_verts);

        STOP_TIME (tm_reduce_set_difference);

        if (undecided_verts.empty ())
          changed = false;
        START_TIME (tm_reduce_update_edges);
        for (auto&& v : undecided_verts)
          nrg_game.update_outs (
            v,
            [&] (typename EnergyGame::neighbors_t::value_type& neighbor) {
              if (neighbor.first >= infty or
                  norm_pot[neighbor.second] >= infty or
                  norm_pot[v] >= infty)
                neighbor.first = infty;
              else if (neighbor.first <= minus_infty or
                       norm_pot[neighbor.second] <= minus_infty or
                       norm_pot[v] <= minus_infty)
                neighbor.first = minus_infty;
              else {
                neighbor.first += norm_pot[neighbor.second];
                neighbor.first -= norm_pot[v]; // two steps to avoid creating a number on the fly
              }
            });
        STOP_TIME (tm_reduce_update_edges);
        STOP_TIME (tm_reduce);
        return changed;
      }

      const potential_t& get_potential () const {
        return potential;
      }
  };
}
