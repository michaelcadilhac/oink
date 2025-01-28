#pragma once

#include "energy_game/types.hpp"
#include "energy_game/numbers.hpp"

template <MovableNumber W, typename... ExtraEdgeInfo>
class energy_game {
    size_t             nverts, nedges;
    W                  infty, minus_infty;
    static constexpr bool     has_extra_edge_info = (sizeof... (ExtraEdgeInfo) > 0);

    using neighbor_t = std::conditional<has_extra_edge_info, std::tuple<vertex_t, ExtraEdgeInfo...>, vertex_t>::type;

    static constexpr auto     get_state = [] (const neighbor_t& n) {
      if constexpr (has_extra_edge_info)
        return std::get<0> (n);
      else
        return n;
    };

    static constexpr auto     make_neighbor = [] (const vertex_t& v) {
      if constexpr (has_extra_edge_info)
        return std::tuple_cat (std::make_tuple (v), std::tuple<ExtraEdgeInfo...> {});
      else
        return v;
    };

  public:
    using weight_t = W;
    using neighbors_t = std::vector<neighbor_t>;

  private:
    std::vector<neighbors_t> out_neighbors, in_neighbors;
    std::vector<bool>  max_owned;
    std::vector<vertex_t> max_verts, min_verts;
    std::vector<weight_t> weights;

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
      weights (nverts),
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
        weights[v] =
          weight_t::copy (
            (prio_to_number.try_emplace (prio,
                                         [&prio, &pgame, &swap] () -> W {
                                           return priority_to_number<typename W::number_t> (prio, pgame, swap);
                                         })).first->second ());

        for (const int* o = pgame.outedges() + pgame.firstout (v); *o != -1; ++o, ++w) {
          out_neighbors[v].push_back (make_neighbor (*o));
          in_neighbors[*o].push_back (make_neighbor (v));
        }

        max_owned[v] = (pgame.owner (v) == 0) ^ swap;
        if (max_owned[v])
          max_verts.push_back (v);
        else
          min_verts.push_back (v);
      }
    }

  public:
    void add_transition (const vertex_t& v1, const vertex_t& v2) {
      out_neighbors[v1].push_back (make_neighbor (v2));
      in_neighbors[v2].push_back (make_neighbor (v1));
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

    neighbors_t& outs (const vertex_t& v) {
      return out_neighbors[v];
    }

    neighbors_t& ins (const vertex_t& v) {
      return in_neighbors[v];
    }

    const W& weight (const vertex_t& v) const { return weights[v]; }
    W& weight (const vertex_t& v) { return weights[v]; }

    size_t size() const {
      return nverts;
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
          os << v << " -> " << std::get<0> (e) << " [label=\"" << std::get<1> (e) << "\"];" << std::endl;
      }
      os << "}" << std::endl;
      return os;
    }
};

template <MovableNumber W, typename... ExtraEdgeInfo>
std::ostream& operator<< (std::ostream& os, const energy_game<W, ExtraEdgeInfo...>& egame) {
  return egame.print (os);
}
