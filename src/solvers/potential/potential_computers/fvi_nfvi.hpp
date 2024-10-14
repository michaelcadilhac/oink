#pragma once

#include "solvers/stats.hpp"
#include "solvers/potential/potential_computers/mutable_priority_queue.hh"

ADD_TO_STATS (nfvi_pot_iter);
ADD_TO_STATS (nfvi_pot_phase1);
ADD_TO_STATS (nfvi_pot_phase2);
ADD_TO_STATS (nfvi_pot_backtrack);

#ifdef NDEBUG
# define log(T)
#else
# define log(T) do { if (this->trace >= 1) { this->logger << T; } } while (0)
#endif


namespace potential {
  template <bool SwapRoles, typename EnergyGame, typename PotentialTeller>
  requires // checks that EnergyGame was defined with extra info on the edges
    std::same_as <std::tuple_element_t<2, typename EnergyGame::neighbors_t::value_type>,
                  typename PotentialTeller::extra_edge_info_t>
  class potential_fvi_nfvi_swap : public potential_computer<EnergyGame, PotentialTeller> {
      using weight_t = EnergyGame::weight_t;
      using neighbor_t = typename EnergyGame::neighbors_t::value_type;
      using potential_computer<EnergyGame, PotentialTeller>::nrg_game;
      using potential_computer<EnergyGame, PotentialTeller>::teller;
      using potential_computer<EnergyGame, PotentialTeller>::potential;
      std::vector<bool> F;
      std::vector<vertex_t> strat;
      std::vector<size_t> nonneg_out_edges_to_Fc; // Nonzero entries will only
      // be for (max ^ SwapRoles)
      // vertices.

      constexpr static auto comp_max =
        [] (const weight_t& w1, const weight_t& w2) { return SwapRoles ? w1 > w2 : w1 < w2; };
      mutable_priority_queue<vertex_t, weight_t, decltype (comp_max)> max_pq;

      constexpr static auto comp_min =
        [] (const weight_t& w1, const weight_t& w2) { return SwapRoles ? w1 < w2 : w1 > w2; };
      mutable_priority_queue<vertex_t, weight_t, decltype (comp_min)> min_outedge_pq;

    public:
      potential_fvi_nfvi_swap (EnergyGame& nrg_game, PotentialTeller& teller, logger_t& logger, int trace) :
        potential_computer<EnergyGame, PotentialTeller> (nrg_game, teller, logger, trace),
        F (nrg_game.size ()),
        strat (nrg_game.size (), -1),
        nonneg_out_edges_to_Fc (nrg_game.size (), 0),
        max_pq (nrg_game.size ()),
        min_outedge_pq (nrg_game.size ()) {
        for (auto& p : potential)
          p = nrg_game.get_infty ();
      }
    private:
      weight_t& W (vertex_t src, neighbor_t& wv) {
        return teller.get_adjusted_weight (src, std::get<0> (wv), std::get<1> (wv), std::get<2> (wv));
      }

      weight_t& W (neighbor_t& wv, vertex_t dst) {
        return teller.get_adjusted_weight (std::get<1> (wv), std::get<0> (wv), dst, std::get<2> (wv));
      }

    public:
#define State(E) std::get<1> (E)

      std::optional<vertex_t> strategy_for (vertex_t v) {
        if ((SwapRoles ^ nrg_game.is_max (v)) and strat[v] != -1)
          return strat[v];

        if ((SwapRoles ^ nrg_game.is_min (v)) and
            (SwapRoles ? teller.get_potential ()[v] > nrg_game.get_minus_infty ()
             : teller.get_potential ()[v] < nrg_game.get_infty ()))
          for (auto&& o : nrg_game.outs (v))
            if (SwapRoles ? (teller.get_potential ()[State (o)] > nrg_game.get_minus_infty () and W (v, o) >= 0)
                : (teller.get_potential ()[State (o)] < nrg_game.get_infty () and W (v, o) <= 0))
              return State (o);
        return std::nullopt;
      }

      std::vector<vertex_t> to_backtrack;
      void compute () {
        /* We start by determining in linear time the set N of vertices from
         * which Min can force to immediately visit an edge of negative weight;
         * these have En+-value 0.  We will successively update a set F
         * containing the set of vertices over which En+ is currently known. We
         * initialise this set F to N.  Note that all remaining Min vertices have
         * only non-negative outgoing edges, and all remaining Max vertices have
         * (at least) a non-negative outgoing edge.
         */
        log ("Initialization of F\n");
        F.assign (nrg_game.size (), true);
        for (auto& p : potential)
          p = SwapRoles ? nrg_game.get_minus_infty () : nrg_game.get_infty ();

        for (auto&& v : teller.undecided_vertices ()) {
          if (SwapRoles ^ nrg_game.is_max (v))
            F[v] = std::ranges::all_of (nrg_game.outs (v), [this, &v] (auto& x) { return SwapRoles ? W (v, x) > 0 : W (v, x) < 0; });
          else
            F[v] = std::ranges::any_of (nrg_game.outs (v), [this, &v] (auto& x) { return SwapRoles ? W (v, x) > 0 : W (v, x) < 0; });
          if (F[v]) {
            potential[v] = zero_number (*nrg_game.get_infty ());
            log ("Putting " << v << " in F, with pot 0.\n");
          }
        }

        auto add_max_vertex_to_pqs = [this] (vertex_t v) {
          weight_t max_succ = SwapRoles ? nrg_game.get_infty () : nrg_game.get_minus_infty ();
          for (auto&& o : nrg_game.outs (v)) {
            if (not F[State (o)] or teller.is_decided (State (o))) continue;

            if (SwapRoles)
              set_if_plus_smaller (*max_succ, *W (v, o), *potential[State (o)]);
            else
              set_if_plus_larger (*max_succ, *W (v, o), *potential[State (o)]);
          }
          min_outedge_pq.set (v, weight_t::proxy (max_succ), max_pq.alway_update);
          max_pq.set (v, weight_t::steal (max_succ), max_pq.alway_update);
        };

        // Extract the minimal transitions going from  Fc to F.
        std::ranges::fill(nonneg_out_edges_to_Fc, 0);
        for (auto&& v : teller.undecided_vertices ()) {
          if (not F[v]) {
            if (nrg_game.is_max (v) ^ SwapRoles) {
              nonneg_out_edges_to_Fc[v] = 0;
              for (auto&& o : nrg_game.outs (v))
                if (not F[State (o)] and (SwapRoles ? W (v, o) <= 0 : W (v, o) >= 0))
                  nonneg_out_edges_to_Fc[v]++;
              if (nonneg_out_edges_to_Fc[v] == 0)
                add_max_vertex_to_pqs (v);
            }
          }
          else {
            for (auto&& i : nrg_game.ins (v)) {
              if (not F[State (i)] and not (nrg_game.is_max (State (i)) ^ SwapRoles)) {
                // Use a weight proxy to avoid duplication.
                min_outedge_pq.set (State (i), weight_t::proxy (const_cast<weight_t&> (W (i, v))),
                                    min_outedge_pq.only_if_higher);
              }
            }
          }
        }

        /* For each vertex from Max, we keep track of the number of edges that
         * go *to* Fc.  When a vertex is added to F, we simply look at its
         * predecessors, and decrease their count.  This allows detecting
         * efficiently which vertices should be added to F (i.e., when their
         * counter is zero).
         */
        auto decrease_preds =
          [this, &add_max_vertex_to_pqs] (vertex_t v) {
            for (auto&& i : nrg_game.ins (v)) {
              if (F[State (i)]) continue;

              if (nrg_game.is_max (State (i)) ^ SwapRoles) { // Predecessor is Max
                if (nonneg_out_edges_to_Fc[State (i)] > 0 and (SwapRoles ? W (i, v) <= 0 : W (i, v) >= 0)) {
                  if (--nonneg_out_edges_to_Fc[State (i)] == 0)
                    add_max_vertex_to_pqs (State (i));
                }
                else if (nonneg_out_edges_to_Fc[State (i)] == 0) {
                  weight_t w = weight_t::copy (W (i, v));
                  w += potential[v];
                  if (min_outedge_pq.set (State (i), weight_t::proxy (w), max_pq.only_if_lower))
                    max_pq.set (State (i), weight_t::steal (w), max_pq.alway_update);
                }
              }
              else { // Predecessor is Min
                weight_t w = weight_t::copy (W (i, v));
                w += potential[v];
                //! Higher means smaller in min_outedge_pq.
                min_outedge_pq.set (State (i), weight_t::steal (w), min_outedge_pq.only_if_higher);
              }

            }
          };

        while (true) {
          TICK (nfvi_pot_iter);
          /* 1. If there is a Max vertex v notin F, all of whose non-negative outgoing
           * edges vv' lead to F, set, En+(v) to be the maximal w(vv') + En+(v'),
           * add v to F, and go back to 1.
           */
          log ("Phase 1.\n");
          while (not min_outedge_pq.empty ()) {
            TICK (nfvi_pot_phase1);
            auto from = min_outedge_pq.top ().key;

            if (F[from]) {
              assert (nrg_game.is_max (from) ^ SwapRoles);
              min_outedge_pq.pop ();
              continue;
            }

            if (nrg_game.is_max (from) ^ SwapRoles)
              break;

            auto weight = weight_t::steal_or_proxy (const_cast<weight_t&> (min_outedge_pq.top ().priority));

            min_outedge_pq.pop ();
            potential[from] = weight_t::steal_or_copy (weight);
            log ("Putting " << from << " in F with pot " << potential[from] << std::endl);
            F[from] = true;
            decrease_preds (from);
          }
          // Treat one max
          if (max_pq.empty ()) // We're done
            break;
          auto from = max_pq.top ().key;
          auto weight = weight_t::steal_or_proxy (const_cast<weight_t&> (max_pq.top ().priority));
          max_pq.pop ();
          // min_outedge_pq.remove (from); // TODO check if better
          assert (not F[from]);
          F[from] = true;
          potential[from] = weight_t::steal_or_copy (weight);
          decrease_preds (from);
        }

        /* After the iteration has terminated, there remains to deal with Fc,
         * which is the set of vertices from which Max can ensure to visit only
         * non-negative edges forever. Since the arena is assumed to be simple
         * (no simple cycle has weight zero) it holds that En+ is ∞ over Fc, and
         * we are done.
         */
        to_backtrack.reserve (teller.undecided_vertices ().size ());
        to_backtrack.clear ();
        for (auto&& v : teller.undecided_vertices ()) {
          if (F[v]) continue;
          to_backtrack.push_back (v);
          if ((SwapRoles ^ nrg_game.is_max (v)) and strat[v] == -1) // Find a strategy
            for (auto&& o : nrg_game.outs (v))
              if (not F[State (o)] and (SwapRoles ? W (v, o) <= 0 : W (v, o) >= 0)) {
                strat[v] = State (o);
                break;
              }
        }

        // This is reused to save a bit of memory.  It will be used for player
        // min ^ SwapRoles, and it is already zero in these locations.
        auto& out_edges_in_F = nonneg_out_edges_to_Fc;

        // In addition, we compute the attractor of the newly discovered decided nodes
        while (not to_backtrack.empty ()) {
          TICK (nfvi_pot_backtrack);
          auto v = to_backtrack.back ();
          to_backtrack.pop_back ();
          for (auto&& wi : nrg_game.ins (v)) {
            auto i = State (wi);
            if (not F[i] or teller.is_decided (i))  // Already treated.
              continue;
            if (SwapRoles ^ nrg_game.is_max (i)) {
              F[i] = false;
              potential[i] = SwapRoles ? nrg_game.get_minus_infty () : nrg_game.get_infty ();
              strat[i] = v;
              to_backtrack.push_back (i);
            }
            else {
              if (out_edges_in_F[i] == 0) // We've not seen that vertex yet
                out_edges_in_F[i] = nrg_game.outs (i).size ();
              if (--out_edges_in_F[i] == 0) { // All of its succ are out of F
                F[i] = false;
                potential[i] = SwapRoles ? nrg_game.get_minus_infty () : nrg_game.get_infty ();
                to_backtrack.push_back (i);
              }
            }
          }
        }

        log ("F =");
        for (auto x : F)
          log (" " << x);
        log ("\n");
      }
  };

  template <typename EG, typename PT>
  using potential_fvi_nfvi = potential_fvi_nfvi_swap<false, EG, PT>;
}

#undef log
#undef State
