#pragma once

#include "solvers/fvi/utils.hpp"
#include "solvers/fvi/stats.hpp"
#include "solvers/fvi/weights.hpp"

ADD_TO_STATS (eg_reduce);
ADD_TO_STATS (eg_pot_update);

template <typename W>
class energy_game;

template <typename W>
std::ostream& operator<< (std::ostream& os, const energy_game<W>& egame) {
  auto&& pot = egame.get_potential ();
  os << "digraph G {" << std::endl;
  for (auto&& v : egame.vertices ()) {
    os << v << " [ shape=\"" << (egame.is_max (v) ? "box" : "circle")
       << "\", label=\"" << v << ",pot=" << pot[v] << "\"";
    if (egame.decided[v]) {
      os << ",bgcolor=" <<
        ((pot[v] >= egame.get_infty ()) == egame.is_max (v)) ? "green" : "red";
    }
    os << "];" << std::endl;
    if (not egame.decided[v])
      for (auto&& e : egame.outs (v))
        os << v << " -> " << e.second << " [label=\"" << e.first << "\"];" << std::endl;

  }
  os << "}" << std::endl;

  return os;
}

template <typename W>
class energy_game {
    bool               changed = true;
    size_t             nverts, nedges;
    W                  infty, minus_infty;
    using potential_t = std::vector<W>;
    potential_t        potential;
    using neighbors_t = std::vector<std::pair<W, vertex_t>>;
    std::vector<neighbors_t> out_neighbors, in_neighbors;
    std::vector<bool>  max_owned;
    std::vector<bool>  decided;
    std::vector<vertex_t> max_verts, min_verts;
    std::set<vertex_t> undecided_verts, undecided_max_verts, undecided_min_verts;

    logger_t& logger;
    int trace = 0;

  public:
    energy_game (const pg::Game& pgame, logger_t& logger, int trace, bool swap = false) :
      nverts (pgame.nodecount ()),
      nedges (pgame.edgecount ()),
      infty (infinity_weight<typename W::number_t> (pgame)),
      minus_infty (-*infty),
      out_neighbors (nverts), in_neighbors (nverts),
      max_owned (nverts),
      decided (nverts),
      logger (logger),
      trace (trace) {
      auto all_verts = std::views::iota ((vertex_t) 0, (vertex_t) nverts);
      // Note: std::ranges::to is for that, but is cxx23.
      undecided_verts = std::set (all_verts.begin (), all_verts.end ());

      potential.reserve (nverts);
      for (size_t i = 0; i < nverts; ++i)
        potential.push_back (zero_weight<typename W::number_t> (*infty));
      reserve (pgame);
      init (pgame, swap);
      // TODO: some nodes are already solved: flush them by backward propagation.
    }

    void reserve (const pg::Game& pgame) {
      for (size_t v = 0; v < nverts; ++v) {
        out_neighbors[v].reserve (pgame.outcount (v));
        in_neighbors[v].reserve (pgame.incount (v));
      }
    }

    void init (const pg::Game& pgame, bool swap) {
      size_t w = 0;
      struct memoized_weight {
          W weight;
          memoized_weight (std::function<W ()> F) : weight (F()) {};
          const W& operator() () const { return weight; }
      };
      std::map<priority_t, memoized_weight> prio_to_weight; // TODO: Turn into vector?
      for (size_t v = 0; v < nverts; ++v) {
        auto prio = pgame.priority (v);
        const W& priow =
          (prio_to_weight.try_emplace (prio,
                                       [&prio, &pgame, &swap] () -> W {
                                         return priority_to_weight<typename W::number_t> (prio, pgame, swap);
                                       })).first->second ();

        for (const int* o = pgame.outedges() + pgame.firstout (v); *o != -1; ++o, ++w) {
          W trans_w = W::copy (priow);
          out_neighbors[v].push_back (std::make_pair (W::steal (trans_w), *o));
          in_neighbors[*o].push_back (std::make_pair (W::proxy (trans_w), v));
        }

        max_owned[v] = (pgame.owner (v) == 0) ^ swap;
        if (max_owned[v]) {
          max_verts.push_back (v);
          undecided_max_verts.insert (v);
        }
        else {
          min_verts.push_back (v);
          undecided_min_verts.insert (v);
        }
      }
    }

  public:
    void add_transition (const vertex_t& v1, const W& w, const vertex_t& v2) {
      W tw = W::copy (w);
      out_neighbors[v1].push_back (std::make_pair (W::steal (tw), v2));
      in_neighbors[v2].push_back (std::make_pair (W::proxy (tw), v1));
      //infty = max (abs (*w) * nverts, *infty); // this is being lazy, but add_transition is used only for debug.
      minus_infty = -*infty;
    }

    void make_max (const vertex_t& v) { max_owned[v] = true; max_verts.push_back (v); }
    void make_min (const vertex_t& v) { max_owned[v] = false; min_verts.push_back (v); }

    bool is_max (const vertex_t& v) const {
      return max_owned[v];
    }
    bool is_min (const vertex_t& v) const {
      return not max_owned[v];
    }

    const neighbors_t& outs (const vertex_t& v) const {
      return out_neighbors[v];
    }

    const neighbors_t& ins (const vertex_t& v) const {
      return in_neighbors[v];
    }

    size_t size() const {
      return nverts;
    }

    auto vertices () const {
      return std::views::iota (0ul, nverts);
    }

    const auto& undecided_vertices () const {
      return undecided_verts;
    }

    const auto& min_vertices () const {
      return min_verts;
    }

    const auto& max_vertices () const {
      return max_verts;
    }

    const auto& undecided_min_vertices () const {
      return undecided_min_verts;
    }

    const auto& undecided_max_vertices () const {
      return undecided_max_verts;
    }


    std::set<vertex_t> newly_decided,
      newly_decided_max, newly_decided_min;

    void reduce (const potential_t& norm_pot) {
      C (eg_reduce);

      changed = false;

      newly_decided.clear (),
        newly_decided_max.clear (), newly_decided_min.clear ();

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
            if (max_owned[v])
              newly_decided_max.insert (v);
            else
              newly_decided_min.insert (v);
            decided[v] = true;
          }
        }
      }

      if (not changed)
        // No need to update, we're done.
        return;

      // Remove decided nodes
      for (auto v : newly_decided) {
        for (auto& n : out_neighbors[v]) {
          if (n.second == v) continue;
          auto& in = in_neighbors[n.second];
          auto pos = std::find_if (in.begin (), in.end (),
                                   [&v] (auto& x) { return x.second == v; });
          assert (pos != in.end ());
          in.erase (pos);
        }
        for (auto& p : in_neighbors[v]) {
          if (p.second == v) continue;
          auto& out = out_neighbors[p.second];
          auto pos = std::find_if (out.begin (), out.end (),
                                   [&v] (auto& x) { return x.second == v; });
          assert (pos != out.end ());
          out.erase (pos);
        }
      }

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
        for (auto& o : out_neighbors[v]) {
          if (o.first >= infty or norm_pot[o.second] >= infty or norm_pot[v] >= infty)
            o.first = infty;
          else if (o.first <= minus_infty or norm_pot[o.second] <= minus_infty or norm_pot[v] <= minus_infty)
            o.first = minus_infty;
          else {
            o.first += norm_pot[o.second];
            o.first -= norm_pot[v]; // two steps to avoid creating a number on the fly
          }
        }
    }

    const potential_t& get_potential () const {
      return potential;
    }

    bool is_decided() const {
      return not changed;
    }

    const W& get_infty () const {
      return infty;
    }
    const W& get_minus_infty () const {
      return minus_infty;
    }

    friend std::ostream& operator<<<> (std::ostream&, const energy_game&);
};
