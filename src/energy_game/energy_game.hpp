#pragma once

#include "energy_game/types.hpp"
#include "energy_game/numbers.hpp"

template <MovableNumber W>
class energy_game;

template <MovableNumber W>
std::ostream& operator<< (std::ostream& os, const energy_game<W>& egame) {
  os << "digraph G {" << std::endl;
  for (auto&& v : egame.vertices ()) {
    os << v << " [ shape=\"" << (egame.is_max (v) ? "box" : "circle")
       << "\", label=\"" << v << "\"";
    os << "];" << std::endl;
    for (auto&& e : egame.outs (v))
      os << v << " -> " << e.second << " [label=\"" << e.first << "\"];" << std::endl;
  }
  os << "}" << std::endl;

  return os;
}

template <MovableNumber W>
class energy_game {
    size_t             nverts, nedges;
    W                  infty, minus_infty;
  public:
    using weight_t = W;
    using neighbors_t = std::vector<std::pair<weight_t, vertex_t>>;

  private:
    std::vector<neighbors_t> out_neighbors, in_neighbors;
    std::vector<bool>  max_owned;
    std::vector<vertex_t> max_verts, min_verts;

    logger_t& logger;
    int trace = 0;

  public:

    energy_game (const pg::Game& pgame, logger_t& logger, int trace, bool swap = false) :
      nverts (pgame.nodecount ()),
      nedges (pgame.edgecount ()),
      infty (infinity_number<typename W::number_t> (pgame)),
      minus_infty (-*infty),
      out_neighbors (nverts), in_neighbors (nverts),
      max_owned (nverts),
      logger (logger),
      trace (trace) {
      // Reserve.
      for (size_t v = 0; v < nverts; ++v) {
        out_neighbors[v].reserve (pgame.outcount (v));
        in_neighbors[v].reserve (pgame.incount (v));
      }

      // Init
      size_t w = 0;
      struct memoized_number {
          W number;
          memoized_number (std::function<W ()> F) : number (F()) {};
          const W& operator() () const { return number; }
      };
      std::map<priority_t, memoized_number> prio_to_number; // TODO: Turn into vector?
      for (size_t v = 0; v < nverts; ++v) {
        auto prio = pgame.priority (v);
        const W& priow =
          (prio_to_number.try_emplace (prio,
                                       [&prio, &pgame, &swap] () -> W {
                                         return priority_to_number<typename W::number_t> (prio, pgame, swap);
                                       })).first->second ();

        for (const int* o = pgame.outedges() + pgame.firstout (v); *o != -1; ++o, ++w) {
          W trans_w = W::copy (priow);
          out_neighbors[v].push_back (std::make_pair (W::steal (trans_w), *o));
          in_neighbors[*o].push_back (std::make_pair (W::proxy (trans_w), v));
        }

        max_owned[v] = (pgame.owner (v) == 0) ^ swap;
        if (max_owned[v])
          max_verts.push_back (v);
        else
          min_verts.push_back (v);
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

    template <std::invocable<typename neighbors_t::value_type&> F>
    void update_outs (const vertex_t& v, const F& f) {
      for (auto& o : out_neighbors[v])
        f (o);
    }

    const W& some_outweight (const vertex_t& v) const {
      return out_neighbors[v][0].first;
    }

    size_t size() const {
      return nverts;
    }

    void isolate_vertex (const vertex_t& v) {
      for (auto& n : out_neighbors[v]) {
        if (n.second == v) continue;
        auto& in = in_neighbors[n.second];
        auto pos = std::find_if (in.begin (), in.end (),
                                 [&v] (auto& x) { return x.second == v; });
        assert (pos != in.end ());
        std::swap (*pos, in.back ());
        in.pop_back ();
      }
      out_neighbors[v].clear ();
      for (auto& p : in_neighbors[v]) {
        if (p.second == v) continue;
        auto& out = out_neighbors[p.second];
        auto pos = std::find_if (out.begin (), out.end (),
                                 [&v] (auto& x) { return x.second == v; });
        assert (pos != out.end ());
        std::swap (*pos, out.back ());
        out.pop_back ();
      }
      in_neighbors[v].clear ();
    }

    bool is_isolated (const vertex_t& v) const {
      return out_neighbors[v].empty () and in_neighbors[v].empty ();
    }

    auto vertices () const {
      return std::views::iota (static_cast<vertex_t> (0),
                               static_cast<vertex_t> (nverts));
    }

    const auto& min_vertices () const {
      return min_verts;
    }

    const auto& max_vertices () const {
      return max_verts;
    }

    const W& get_infty () const {
      return infty;
    }
    const W& get_minus_infty () const {
      return minus_infty;
    }

    friend std::ostream& operator<<<> (std::ostream&, const energy_game&);
};
