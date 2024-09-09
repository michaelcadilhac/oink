#pragma once

#include "solvers/potential/stats.hpp"

ADD_TO_STATS (eg_reduce);
ADD_TO_STATS (eg_pot_update);

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
        undecided_max_verts = std::set (nrg_game.max_vertices ().begin (), nrg_game.max_vertices ().end ());
        undecided_min_verts = std::set (nrg_game.min_vertices ().begin (), nrg_game.min_vertices ().end ());
        // TODO: some nodes are already solved: flush them by backward propagation.
      }

      const auto& undecided_min_vertices () const {
        return undecided_min_verts;
      }

      const auto& undecided_max_vertices () const {
        return undecided_max_verts;
      }

      const auto& undecided_vertices () const {
        return undecided_verts;
      }

      std::set<vertex_t> newly_decided,
        newly_decided_max, newly_decided_min;

      bool reduce (const potential_t& norm_pot) {
        C (eg_reduce);

        bool changed = false;

        newly_decided.clear ();
        newly_decided_max.clear ();
        newly_decided_min.clear ();

        for (auto&& v : undecided_verts) {
          if (not decided[v] and norm_pot[v] != 0) {
            C (eg_pot_update);
            changed = true;
            if (norm_pot[v] >= infty)
              potential[v] = infty;
            else if (norm_pot[v] <= minus_infty)
              potential[v] = minus_infty;
            else
              potential[v] += norm_pot[v];
            if (potential[v] >= infty or potential[v] <= minus_infty) {
              newly_decided.insert (v);
              if (nrg_game.is_max (v))
                newly_decided_max.insert (v);
              else
                newly_decided_min.insert (v);
              decided[v] = true;
            }
          }
        }

        if (not changed)
          // No need to update, we're done.
          return changed;

        // Remove decided nodes
        for (auto v : newly_decided)
          nrg_game.isolate_vertex (v);

        std::set<vertex_t> result;
        std::set_difference (undecided_verts.begin(), undecided_verts.end(),
                             newly_decided.begin(), newly_decided.end(),
                             std::inserter (result, result.end()));
        std::swap (result, undecided_verts);

        result.clear ();
        std::set_difference (undecided_max_verts.begin(), undecided_max_verts.end(),
                             newly_decided_max.begin(), newly_decided_max.end(),
                             std::inserter (result, result.end()));
        std::swap (result, undecided_max_verts);

        result.clear ();
        std::set_difference (undecided_min_verts.begin(), undecided_min_verts.end(),
                             newly_decided_min.begin(), newly_decided_min.end(),
                             std::inserter (result, result.end()));
        std::swap (result, undecided_min_verts);

        if (undecided_verts.empty ())
          changed = false;
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
        return changed;
      }

      const potential_t& get_potential () const {
        return potential;
      }
  };
}
