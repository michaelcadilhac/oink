#pragma once

#include "solvers/potential/stats.hpp"
#include "solvers/potential/potential_computers/mutable_priority_queue.hh"

ADD_TO_STATS (pot_compute);
ADD_TO_STATS (pot_iter);
ADD_TO_STATS (pot_phase1);
ADD_TO_STATS (pot_phase2);
ADD_TO_STATS (pot_backtrack);

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
  class potential_fvi_swap : public potential_computer<EnergyGame, PotentialTeller> {
      using weight_t = EnergyGame::weight_t;
      using neighbor_t = EnergyGame::neighbors_t::value_type;
      using potential_computer<EnergyGame, PotentialTeller>::nrg_game;
      using potential_computer<EnergyGame, PotentialTeller>::teller;
      using potential_computer<EnergyGame, PotentialTeller>::potential;
      std::vector<bool> F;
      std::vector<vertex_t> strat;
      std::vector<size_t> nonneg_out_edges_to_Fc; // Nonzero entries will only
      // be for (max ^ SwapRoles)
      // vertices.
    public:
      potential_fvi_swap (EnergyGame& nrg_game, PotentialTeller& teller, logger_t& logger, int trace) :
        potential_computer<EnergyGame, PotentialTeller> (nrg_game, teller, logger, trace),
        F (nrg_game.size ()),
        strat (nrg_game.size (), -1),
        nonneg_out_edges_to_Fc (nrg_game.size (), 0) {
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

      int32_t WS (vertex_t src, neighbor_t& wv) {
        return teller.get_adjusted_weight_sign (src, std::get<0> (wv), std::get<1> (wv), std::get<2> (wv));
      }

      int32_t WS (neighbor_t& wv, vertex_t dst) {
        return teller.get_adjusted_weight_sign (std::get<1> (wv), std::get<0> (wv), dst, std::get<2> (wv));
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
            if (SwapRoles ?
                (teller.get_potential ()[State (o)] > nrg_game.get_minus_infty () and WS (v, o) >= 0)
                : (teller.get_potential ()[State (o)] < nrg_game.get_infty () and WS (v, o) <= 0))
              return State (o);
        return std::nullopt;
      }

      std::vector<vertex_t> to_backtrack;
      void compute () {
        C (pot_compute);

        /* We start by determining in linear time the set N of vertices from
         * which Min can force to immediately visit an edge of negative weight;
         * these have En+-value 0.  We will successively update a set F
         * containing the set of vertices over which En+ is currently known. We
         * initialise this set F to N.  Note that all remaining Min vertices have
         * only non-negative outgoing edges, and all remaining Max vertices have
         * (at least) a non-negative outgoing edge.
         */
        log ("Initialization of F\n");
        // All the decided vertices are marked true, in particular.
        F.assign (nrg_game.size (), true);
        for (auto& p : potential)
          p = SwapRoles ? nrg_game.get_minus_infty () : nrg_game.get_infty ();

        for (auto&& v : teller.undecided_vertices ()) {
          if (SwapRoles ^ nrg_game.is_max (v))
            F[v] = std::ranges::all_of (nrg_game.outs (v), [this, &v] (auto& x) { return SwapRoles ? WS (v, x) > 0 : WS (v, x) < 0; });
          else
            F[v] = std::ranges::any_of (nrg_game.outs (v), [this, &v] (auto& x) { return SwapRoles ? WS (v, x) > 0 : WS (v, x) < 0; });
          if (F[v]) {
            potential[v] = zero_number (*nrg_game.get_infty ());
            log ("Putting " << v << " in F, with pot 0.\n");
          }
        }

        std::queue<vertex_t> phase1_queue;
        auto comp = [] (const weight_t& w1, const weight_t& w2) { return SwapRoles ? w1 < w2 : w1 > w2; };
        auto phase2_pq = mutable_priority_queue<vertex_t, weight_t, decltype (comp)> (nrg_game.size ());

        // Extract the minimal transitions going from  Fc to F.
        std::ranges::fill(nonneg_out_edges_to_Fc, 0);
        for (auto&& v : teller.undecided_vertices ()) {
          if (not F[v]) {
            if (nrg_game.is_max (v) ^ SwapRoles) {
              nonneg_out_edges_to_Fc[v] = 0;
              for (auto&& o : nrg_game.outs (v))
                if (not F[State (o)] and (SwapRoles ? WS (v, o) <= 0 : WS (v, o) >= 0))
                  nonneg_out_edges_to_Fc[v]++;
              if (nonneg_out_edges_to_Fc[v] == 0)
                phase1_queue.push (v);
            }
          }
          else {
            for (auto&& i : nrg_game.ins (v)) {
              if (not F[State (i)] and not (nrg_game.is_max (State (i)) ^ SwapRoles)) {
                // Use a weight proxy to avoid duplication.
                phase2_pq.set (State (i), weight_t::proxy (W (i, v)),
                               true);
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
          [this, &phase1_queue, &phase2_pq] (vertex_t v) {
            for (auto&& i : nrg_game.ins (v)) {
              if (not F[State (i)] and not (nrg_game.is_max (State (i)) ^ SwapRoles)) {
                weight_t w = weight_t::copy (W (i, v));
                w += potential[v];
                phase2_pq.set (State (i), weight_t::steal (w), true); //!! should be steal
              }
              if (nonneg_out_edges_to_Fc[State (i)] and (SwapRoles ? WS (i, v) <= 0 : WS (i, v) >= 0)) {
                assert ((SwapRoles ^ nrg_game.is_max (State (i))) and not F[State (i)]);
                if (--nonneg_out_edges_to_Fc[State (i)] == 0)
                  phase1_queue.push (State (i));
              }
            }
          };

        while (true) {
          C (pot_iter);
          /* 1. If there is a Max vertex v notin F, all of whose non-negative outgoing
           * edges vv' lead to F, set, En+(v) to be the maximal w(vv') + En+(v'),
           * add v to F, and go back to 1.
           */
          log ("Phase 1.\n");
          while (not phase1_queue.empty ()) {
            C (pot_phase1);
            vertex_t v = phase1_queue.front ();
            phase1_queue.pop ();
            assert (not F[v] and nonneg_out_edges_to_Fc[v] == 0);
            potential[v] = SwapRoles ? nrg_game.get_infty () : nrg_game.get_minus_infty ();
            for (auto&& o : nrg_game.outs (v)) {
              if (teller.is_decided (State (o))) continue;
              if (SwapRoles ? WS (v, o) > 0 : WS (v, o) < 0) continue;
              assert (F[State (o)]);
              if (SwapRoles)
                set_if_plus_smaller (*potential[v], *W (v, o), *potential[State (o)]);
              else
                set_if_plus_larger (*potential[v], *W (v, o), *potential[State (o)]);
            }
            log ("Putting " << v << " in F with pot " << potential[v] << std::endl);
            F[v] = true;
            decrease_preds (v);
          }

          /* 2. Otherwise, let vv' be an edge from VMin \ F to F (it is
           * necessarily positive) minimising w(vv') + En+(v'); set En+(v) =
           * w(vv') + En+(v'), add v to F and go back to 1. If there is no such
           * edge, terminate.  For step 2, one should store, for each v ∈ VMax \
           * F, the edge towards F minimising w(vv') + En+(v') in a priority
           * queue.
           */
          log ("Phase 2.\n");
          bool change = false;
          while (not phase2_pq.empty ()) {
            C (pot_phase2);
            auto from = phase2_pq.top ().key;
            auto weight = weight_t::steal_or_proxy (const_cast<weight_t&> (phase2_pq.top ().priority));
            phase2_pq.pop ();
            if (F[from]) continue;
            potential[from] = weight_t::steal_or_copy (weight);
            F[from] = true;
            log ("Putting " << from << " in F with pot " << potential[from] << std::endl);
            decrease_preds (from);
            change = true;
            break;
          }
          if (not change)
            break;
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
              if (not F[State (o)] and (SwapRoles ? WS (v, o) <= 0 : WS (v, o) >= 0)) {
                strat[v] = State (o);
                break;
              }
        }

        // This is reused to save a bit of memory.  It will be used for player
        // min ^ SwapRoles, and it is already zero in these locations.
        auto& out_edges_in_F = nonneg_out_edges_to_Fc;

        // In addition, we compute the attractor of the newly discovered decided nodes
        while (not to_backtrack.empty ()) {
          C (pot_backtrack);
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
  using potential_fvi = potential_fvi_swap<false, EG, PT>;
}

#undef log
#undef State
