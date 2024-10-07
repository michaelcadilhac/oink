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
  class potential_teller {
      using weight_t = typename EnergyGame::weight_t;
      using potential_t = std::vector<weight_t>;

      EnergyGame& nrg_game;
      const weight_t& infty, minus_infty;
      std::vector<bool>  decided;

      potential_t potential;
      std::set<vertex_t> undecided_verts;

      std::vector<std::pair<size_t, std::optional<weight_t>>> edge_timestamps;
      std::vector<size_t> vert_timestamps;

      size_t time;

    public:
      potential_teller (EnergyGame& ngame) : nrg_game { ngame },
                                             infty { ngame.get_infty () },
                                             minus_infty { ngame.get_minus_infty () },
                                             decided (ngame.size ()),
                                             edge_timestamps (ngame.size () * ngame.size ()),
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

      weight_t& get_adjusted_weight (vertex_t p, weight_t& w, vertex_t q) {
        START_TIME (tm_teller_edge_search);
        auto& e_ts = edge_timestamps[p * nrg_game.size () + q];
        auto& p_ts = vert_timestamps[p];
        auto& q_ts = vert_timestamps[q];
        STOP_TIME (tm_teller_edge_search);

        if (e_ts.first > p_ts and e_ts.first > q_ts)
          return *e_ts.second;

        if ((p_ts == 0 and q_ts == 0) or potential[p] == potential[q]) {// original value
          e_ts.first = std::max (p_ts, q_ts) + 1; // Time 1 is specifically reserved for initialization.
          e_ts.second = weight_t::proxy (w);
        }
        else  {
          if (potential[q] >= infty or potential[p] >= infty) {
            e_ts.first = SIZE_MAX;
            e_ts.second = infty;
          }
          else if (potential[q] <= minus_infty or potential[p] <= minus_infty) {
            e_ts.first = SIZE_MAX;
            e_ts.second = minus_infty;
          }
          else {
            e_ts.first = std::max (p_ts, q_ts) + 1;
            e_ts.second = weight_t::copy (w);
            *e_ts.second += potential[q];
            *e_ts.second -= potential[p];
          }
        }
        return *e_ts.second;
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
