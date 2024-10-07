#pragma once

#include "solvers/potential/stats.hpp"

ADD_TO_STATS (eg_reduce);
ADD_TO_STATS (eg_pot_update);
ADD_TO_STATS (pot_sign);
ADD_TO_STATS (pot_sign_recompute);

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

      std::vector<std::tuple <size_t, size_t, size_t>> vert_timestamps;

#define TS_LAST_DEC(TS) std::get<0> (TS)
#define TS_LAST_INC(TS) std::get<1> (TS)
#define TS_LAST_MOD(TS) std::get<2> (TS)

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

      weight_t& recompute_weight (vertex_t p, weight_t& w, vertex_t q,
                                  size_t& edge_timestamp, weight_t& adjusted_weight,
                                  int32_t& sign) {
        if (potential[q] >= infty or potential[p] >= infty) {
          edge_timestamp = SIZE_MAX;
          adjusted_weight = infty;
          sign = 1;
        }
        else if (potential[q] <= minus_infty or potential[p] <= minus_infty) {
          edge_timestamp = SIZE_MAX;
          adjusted_weight = minus_infty;
          sign = -1;
        }
        else {
          edge_timestamp = time + 1;
          adjusted_weight = weight_t::copy (w);
          adjusted_weight += potential[q];
          adjusted_weight -= potential[p];
          sign = (*adjusted_weight).sign ();
        }

        return adjusted_weight;
      }

      int32_t get_adjusted_weight_sign (vertex_t p, weight_t& w, vertex_t q,
                                        size_t& edge_timestamp, weight_t& adjusted_weight,
                                        int32_t& sign) {
        auto& p_ts = vert_timestamps[p];
        auto& q_ts = vert_timestamps[q];

        C (pot_sign);

        if (edge_timestamp > TS_LAST_MOD (p_ts) and edge_timestamp > TS_LAST_MOD (q_ts)) // no modification
          return sign;

        if (TS_LAST_MOD (p_ts) == 0 and TS_LAST_MOD (q_ts) == 0) {
          // original value
          edge_timestamp = time + 1;
          adjusted_weight = weight_t::proxy (w);
          sign = (*adjusted_weight).sign ();
          return sign;
        }

        // Needs recomputing.  Before we do, let's see if we can answer the
        // question just with timestamps.
        if (sign > 0) {
          // If the update + q - p has increased since edge_timestamp, then
          // potential must stay positive.  This happens in particular if there
          // were no decrease of q or increase of p since edge_timestamp.
          if (TS_LAST_DEC (q_ts) < edge_timestamp and TS_LAST_INC (p_ts) < edge_timestamp)
            return sign;
        }
        else if (sign < 0) {
          // If the update + q - p has decreased since edge_timestamp, then
          // potential must stay <= 0.  This happens in particular if there
          // were no increase of q or decrease of p since edge_timestamp.
          if (TS_LAST_INC (q_ts) < edge_timestamp and TS_LAST_DEC (p_ts) < edge_timestamp)
            return sign;
        }

        C (pot_sign_recompute);
        // Needs actual recomputing
        recompute_weight (p, w, q, edge_timestamp, adjusted_weight, sign);
        return sign;
      }

      weight_t& get_adjusted_weight (vertex_t p, weight_t& w, vertex_t q,
                                     size_t& edge_timestamp, weight_t& adjusted_weight, int32_t& sign) {
        auto& p_ts = vert_timestamps[p];
        auto& q_ts = vert_timestamps[q];

        if (edge_timestamp > TS_LAST_MOD (p_ts) and edge_timestamp > TS_LAST_MOD (q_ts))
          return adjusted_weight;

        if (TS_LAST_MOD (p_ts) == 0 and TS_LAST_MOD (q_ts) == 0) {
          edge_timestamp = time + 1; // Time 1 is specifically reserved for initialization.
          adjusted_weight = weight_t::proxy (w);
          sign = (*adjusted_weight).sign ();
        }
        else
          recompute_weight (p, w, q, edge_timestamp, adjusted_weight, sign);
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
            auto& ts = vert_timestamps[v];
            C (eg_pot_update);
            TS_LAST_MOD (ts) = time;
            changed = true;
            if (norm_pot[v] >= infty) {
              potential[v] = infty;
              TS_LAST_INC (ts) = time;
            }
            else if (norm_pot[v] <= minus_infty) {
              potential[v] = minus_infty;
              TS_LAST_DEC (ts) = time;
            }
            else {
              potential[v] += norm_pot[v];
              if (norm_pot[v] > 0)
                TS_LAST_INC (ts) = time;
              else
                TS_LAST_DEC (ts) = time;
            }
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
