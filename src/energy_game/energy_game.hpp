#pragma once

#include "energy_game/types.hpp"
#include "energy_game/numbers.hpp"

template <MovableNumber W, typename... ExtraEdgeInfo>
class energy_game {
    size_t             nverts, nedges;
    W                  infty, minus_infty;
  public:
    using weight_t = W;
    using neighbors_t = std::vector<std::tuple<weight_t, vertex_t, ExtraEdgeInfo...>>;

  private:
    std::vector<neighbors_t> out_neighbors, in_neighbors;
    std::vector<bool>  max_owned;
    std::vector<vertex_t> max_verts, min_verts;

#ifndef NOINK
  public:
    logger_t& logger;
    int trace = 0;
#endif

  public:

#ifndef NOINK
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
          out_neighbors[v].push_back (std::tuple_cat (std::make_pair (W::steal (trans_w), *o),
                                                      std::tuple<ExtraEdgeInfo...> {}));
          in_neighbors[*o].push_back (std::tuple_cat (std::make_pair (W::proxy (trans_w), v),
                                                      std::tuple<ExtraEdgeInfo...> {}));
        }

        max_owned[v] = (pgame.owner (v) == 0) ^ swap;
        if (max_owned[v])
          max_verts.push_back (v);
        else
          min_verts.push_back (v);
      }
    }
#else
    energy_game () : nverts (0) {}
#endif

  public:
    void add_state (const vertex_t& v) {
      nverts = std::max (nverts, static_cast<size_t> (v + 1));
      if (out_neighbors.size () < nverts) {
        out_neighbors.resize (nverts);
        in_neighbors.resize (nverts);
        max_owned.resize (nverts);
      }
    }

    void add_transition (const vertex_t& v1, const W& w, const vertex_t& v2) {
      W tw = W::copy (w);
      assert (static_cast<size_t> (v1) < out_neighbors.size ());
      assert (static_cast<size_t> (v2) < in_neighbors.size ());
      out_neighbors[v1].push_back (std::tuple_cat (std::make_pair (W::steal (tw), v2),
                                                   std::tuple<ExtraEdgeInfo...> {}));
      in_neighbors[v2].push_back (std::tuple_cat (std::make_pair (W::proxy (tw), v1),
                                                  std::tuple<ExtraEdgeInfo...> {}));
    }

    void set_infty (const W& w) {
      infty = w;
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

    neighbors_t& outs (const vertex_t& v) {
      return out_neighbors[v];
    }

    neighbors_t& ins (const vertex_t& v) {
      return in_neighbors[v];
    }

    template <std::invocable<typename neighbors_t::value_type&> F>
    void update_outs (const vertex_t& v, const F& f) {
      for (auto& o : out_neighbors[v])
        f (o);
    }

    const W& some_outweight (const vertex_t& v) const {
      return std::get<0> (out_neighbors[v][0]);
    }

    size_t size() const {
      return nverts;
    }

    void isolate_vertex (const vertex_t& v) {
      for (auto& n : out_neighbors[v]) {
        if (std::get<1> (n) == v) continue;
        auto& in = in_neighbors[std::get<1> (n)];
        auto pos = std::find_if (in.begin (), in.end (),
                                 [&v] (auto& x) { return std::get<1> (x) == v; });
        assert (pos != in.end ());
        std::swap (*pos, in.back ());
        in.pop_back ();
      }
      out_neighbors[v].clear ();
      for (auto& p : in_neighbors[v]) {
        if (std::get<1> (p) == v) continue;
        auto& out = out_neighbors[std::get<1> (p)];
        auto pos = std::find_if (out.begin (), out.end (),
                                 [&v] (auto& x) { return std::get<1> (x) == v; });
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

    std::ostream& print (std::ostream& os) const {
      os << "digraph G {" << std::endl;
      for (auto&& v : vertices ()) {
        os << v << " [ shape=\"" << (is_max (v) ? "box" : "circle")
           << "\", label=\"" << v << "\"";
        os << "];" << std::endl;
        for (auto&& e : out_neighbors[v])
          os << v << " -> " << std::get<1> (e) << " [label=\"" << std::get<0> (e) << "\"];" << std::endl;
      }
      os << "}" << std::endl;
      return os;
    }
};

template <MovableNumber W, typename... ExtraEdgeInfo>
std::ostream& operator<< (std::ostream& os, const energy_game<W, ExtraEdgeInfo...>& egame) {
  return egame.print (os);
}
