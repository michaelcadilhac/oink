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

  template <MovableNumber W>
  struct extra_edge_info {
      W adjusted_weight;
      size_t timestamp;
  };

  template <typename EnergyGame>
  class potential_teller {
    public:
      using weight_t = typename EnergyGame::weight_t;
      using potential_t = std::vector<weight_t>;
      using extra_edge_info_t = extra_edge_info<weight_t>;

    private:
      EnergyGame& nrg_game;
      const weight_t infty, minus_infty;
      std::vector<bool>  decided;

      potential_t potential;
      bool changed;

      std::set<vertex_t> undecided_verts;

      std::vector<size_t> vert_timestamps;
      size_t time;

    public:

      potential_teller (EnergyGame& ngame) : nrg_game { ngame },
                                             infty { weight_t::proxy_unsafe (ngame.get_infty ()) },
                                             minus_infty { weight_t::proxy_unsafe (ngame.get_minus_infty ()) },
                                             decided (ngame.size ()),
                                             changed (false),
                                             vert_timestamps (ngame.size ()),
                                             time (2) // time 1 is for initialization
      {
        potential.reserve (nrg_game.size ());
        for (size_t i = 0; i < nrg_game.size (); ++i)
          potential.push_back (zero_number<typename weight_t::number_t> (*infty));

        undecided_verts = std::set (nrg_game.vertices ().begin (), nrg_game.vertices ().end ());
        // TODO: some nodes are already solved: flush them by backward propagation.
      }

      const auto& undecided_vertices () const {
        return undecided_verts;
      }

      weight_t& get_adjusted_weight (vertex_t p, vertex_t q, extra_edge_info_t& ei) {
        weight_t& w = nrg_game.weight (p);
        auto& p_ts = vert_timestamps[p];
        auto& q_ts = vert_timestamps[q];

        if (ei.timestamp > p_ts and ei.timestamp > q_ts)
          return ei.adjusted_weight;

        if ((p_ts == 0 and q_ts == 0) or potential[p] == potential[q]) {// original value
          ei.timestamp = std::max (p_ts, q_ts) + 1; // Time 1 is specifically reserved for initialization.
          ei.adjusted_weight = weight_t::proxy (w);
        }
        else  {
          if (potential[q] >= infty or potential[p] >= infty) {
            ei.timestamp = SIZE_MAX;
            ei.adjusted_weight = weight_t::proxy_unsafe (infty);
          }
          else if (potential[q] <= minus_infty or potential[p] <= minus_infty) {
            ei.timestamp = SIZE_MAX;
            ei.adjusted_weight = weight_t::proxy_unsafe (minus_infty);
          }
          else {
            ei.timestamp = std::max (p_ts, q_ts) + 1;
            ei.adjusted_weight = weight_t::copy (w);
            ei.adjusted_weight += potential[q];
            ei.adjusted_weight -= potential[p];
          }
        }
        return ei.adjusted_weight;
      }

      bool is_decided (const vertex_t& v) { return decided[v]; }

      std::set<vertex_t> newly_decided;

      bool reduce () {
        if (not changed)
          // No need to update, we're done.
          return false;

        TICK (eg_reduce);
        START_TIME (tm_reduce);

        for (auto&& v : newly_decided)
          decided[v] = true;

        START_TIME (tm_reduce_set_difference);
        std::set<vertex_t> result;
        std::set_difference (undecided_verts.begin(), undecided_verts.end(),
                             newly_decided.begin(), newly_decided.end(),
                             std::inserter (result, result.end()));
        std::swap (result, undecided_verts);


        STOP_TIME (tm_reduce_set_difference);

        // Reset everything
        newly_decided.clear ();
        changed = false;

        STOP_TIME (tm_reduce);
        return true;
      }
      const weight_t& get_potential (vertex_t v) const {
        return potential[v];
      }

    private:

      void potential_changed (vertex_t v) {
        vert_timestamps[v] = ++time;
        changed = true;
        if (potential[v] >= infty)
          potential[v] = weight_t::copy (infty);
        else if (potential[v] <= minus_infty)
          potential[v] = weight_t::copy (minus_infty);
        if (potential[v] >= infty or potential[v] <= minus_infty)
          newly_decided.insert (v);
      }

    public:

      bool has_changed () const { return changed; }

      void set_potential (vertex_t v, weight_t&& w) {
        if (potential[v] != w) {
          potential[v] = std::move (w);
          potential_changed (v);
        }
      }

      void inc_potential (vertex_t v, const weight_t& w) {
        if (w != 0) {
          assert (potential[v].is_owning ());
          potential[v] += w;
          potential_changed (v);
        }
      }

      std::ostream& print (std::ostream& os) {
        os << "digraph G {" << std::endl;
        for (auto&& v : nrg_game.vertices ()) {
          os << v << " [ shape=\"" << (nrg_game.is_max (v) ? "box" : "circle")
             << "\", label=\"" << v << "[p=" << potential[v] << "]\"";
          os << "];" << std::endl;
          for (auto&& e : nrg_game.outs (v))
            os << v << " -> " << std::get<0> (e)
               << " [label=\"" << get_adjusted_weight (v, std::get<0> (e), std::get<1> (e))
               << "\"];" << std::endl;
        }
        os << "}" << std::endl;
        return os;
      }
  };
}

template <typename EnergyGame>
std::ostream& operator<< (std::ostream& os, potential::potential_teller<EnergyGame>& t) {
  return t.print (os);
}
