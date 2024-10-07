#pragma once

#include "solvers/potential/stats.hpp"

ADD_TO_STATS (eg_reduce);
ADD_TO_STATS (eg_pot_update);

ADD_TIME_TO_STATS (tm_teller_edge_search);
ADD_TIME_TO_STATS (tm_reduce);
ADD_TIME_TO_STATS (tm_reduce_update_pot);
ADD_TIME_TO_STATS (tm_reduce_isolate);
ADD_TIME_TO_STATS (tm_reduce_set_difference);
ADD_TIME_TO_STATS (tm_reduce_update_edges);

namespace potential {
  template <typename EnergyGame>
  requires requires (EnergyGame& g) {
        { std::get<2> (*g.outs (0).begin ()) } -> std::same_as <size_t&>;  // has timestamp
        { std::get<3> (*g.outs (0).begin ()) } -> std::same_as <typename EnergyGame::weight_t&>;  // has new weight
  }
  class potential_teller {
      using weight_t = typename EnergyGame::weight_t;
      using potential_t = std::vector<weight_t>;

      EnergyGame& nrg_game;
      const weight_t& infty, minus_infty;
      std::vector<bool>  decided;

      potential_t potential;
      std::set<vertex_t> undecided_verts;

      std::vector<size_t> vert_timestamps;

      size_t time;

    public:
      potential_teller (EnergyGame& ngame) : nrg_game { ngame },
                                             infty { ngame.get_infty () },
                                             minus_infty { ngame.get_minus_infty () },
                                             decided (ngame.size ()),
                                             vert_timestamps (ngame.size ()),
                                             time (1) {
        potential.reserve (nrg_game.size ());
        for (size_t i = 0; i < nrg_game.size (); ++i)
          potential.push_back (zero_number<typename weight_t::number_t> (*infty));

        undecided_verts = std::set (nrg_game.vertices ().begin (), nrg_game.vertices ().end ());
        // TODO: some nodes are already solved: flush them by backward propagation.
      }

      const auto& undecided_vertices () const {
        return undecided_verts;
      }

      weight_t& get_adjusted_weight (vertex_t p, weight_t& w, vertex_t q,
                                     size_t& edge_timestamp, weight_t& adjusted_weight) {
        auto& p_ts = vert_timestamps[p];
        auto& q_ts = vert_timestamps[q];

        if (edge_timestamp > p_ts and edge_timestamp > q_ts)
          return adjusted_weight;

        if ((p_ts == 0 and q_ts == 0) or potential[p] == potential[q]) {// original value
          edge_timestamp = std::max (p_ts, q_ts) + 1; // Time 1 is specifically reserved for initialization.
          adjusted_weight = weight_t::proxy (w);
        }
        else  {
          if (potential[q] >= infty or potential[p] >= infty) {
            edge_timestamp = SIZE_MAX;
            adjusted_weight = infty;
          }
          else if (potential[q] <= minus_infty or potential[p] <= minus_infty) {
            edge_timestamp = SIZE_MAX;
            adjusted_weight = minus_infty;
          }
          else {
            edge_timestamp = std::max (p_ts, q_ts) + 1;
            adjusted_weight = weight_t::copy (w);
            adjusted_weight += potential[q];
            adjusted_weight -= potential[p];
          }
        }
        return adjusted_weight;
      }

      bool is_decided (const vertex_t& v) { return decided[v]; }

      std::set<vertex_t> newly_decided;

      bool reduce (const potential_t& norm_pot) {
        ++time;
        C (eg_reduce);
        START_TIME (tm_reduce_update_pot);
        START_TIME (tm_reduce);

        bool changed = false;

        newly_decided.clear ();

        for (auto&& v : undecided_verts) {
          if (not decided[v] and norm_pot[v] != 0) {
            C (eg_pot_update);
            vert_timestamps[v] = time;
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

        START_TIME (tm_reduce_set_difference);
        std::set<vertex_t> result;
        std::set_difference (undecided_verts.begin(), undecided_verts.end(),
                             newly_decided.begin(), newly_decided.end(),
                             std::inserter (result, result.end()));
        std::swap (result, undecided_verts);

        STOP_TIME (tm_reduce_set_difference);

        if (undecided_verts.empty ())
          changed = false;

        STOP_TIME (tm_reduce);
        return changed;
      }

      const potential_t& get_potential () const {
        return potential;
      }
  };
}
