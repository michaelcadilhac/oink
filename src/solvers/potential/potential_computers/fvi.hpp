#pragma once

#include "solvers/stats.hpp"
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
  class potential_fvi_swap : public potential_computer<EnergyGame, PotentialTeller> {
      using weight_t = EnergyGame::weight_t;
      using potential_computer<EnergyGame, PotentialTeller>::nrg_game;
      using potential_computer<EnergyGame, PotentialTeller>::teller;
      using potential_computer<EnergyGame, PotentialTeller>::potential;
      std::vector<bool> F;
      std::vector<vertex_t> strat;
      std::vector<size_t> nonneg_out_edges_to_Fc; // Nonzero entries will only
      // be for (max ^ SwapRoles)
      // vertices.
    public:
      potential_fvi_swap (const EnergyGame& nrg_game, const PotentialTeller& teller, logger_t& logger, int trace) :
        potential_computer<EnergyGame, PotentialTeller> (nrg_game, teller, logger, trace),
        F (nrg_game.size ()),
        strat (nrg_game.size (), -1),
        nonneg_out_edges_to_Fc (nrg_game.size (), 0) {
        for (auto& p : potential)
          p = nrg_game.get_infty ();
      }

      std::optional<vertex_t> strategy_for (vertex_t v) {
        if ((SwapRoles ^ nrg_game.is_max (v)) and strat[v] != -1)
          return strat[v];

        if ((SwapRoles ^ nrg_game.is_min (v)) and
            (SwapRoles ? teller.get_potential ()[v] > nrg_game.get_minus_infty ()
             : teller.get_potential ()[v] < nrg_game.get_infty ()))
          for (auto&& o : nrg_game.outs (v))
            if (SwapRoles ? (teller.get_potential ()[o.second] > nrg_game.get_minus_infty () and o.first >= 0)
                : (teller.get_potential ()[o.second] < nrg_game.get_infty () and o.first <= 0))
              return o.second;
        return std::nullopt;
      }

      std::vector<vertex_t> to_backtrack;
      void compute () {
        TICK (pot_compute);

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
            F[v] = std::ranges::all_of (nrg_game.outs (v), [this] (auto& x) { return SwapRoles ? x.first > 0 : x.first < 0; });
          else
            F[v] = std::ranges::any_of (nrg_game.outs (v), [this] (auto& x) { return SwapRoles ? x.first > 0 : x.first < 0; });
          if (F[v]) {
            potential[v] = zero_number (*nrg_game.get_infty ());
            log ("Putting " << v << " in F, with pot 0.\n");
          }
        }

        auto comp1 = [] (const weight_t& w1, const weight_t& w2) { return SwapRoles ? w1 > w2 : w1 < w2; };
        auto phase1_pq = mutable_priority_queue<vertex_t, weight_t, decltype (comp1)> (nrg_game.size ());
        auto comp2 = [] (const weight_t& w1, const weight_t& w2) { return SwapRoles ? w1 < w2 : w1 > w2; };
        auto phase2_pq = mutable_priority_queue<vertex_t, weight_t, decltype (comp2)> (nrg_game.size ());

        auto add_vertex_to_phase1_pq = [this, &phase1_pq] (vertex_t v) {
          weight_t max_succ = SwapRoles ? nrg_game.get_infty () : nrg_game.get_minus_infty ();
          for (auto&& o : nrg_game.outs (v)) {
            if (not F[o.second]) continue;

            if (SwapRoles)
              set_if_plus_smaller (*max_succ, *o.first, *potential[o.second]);
            else
              set_if_plus_larger (*max_succ, *o.first, *potential[o.second]);
          }
          phase1_pq.set (v, std::move (max_succ), phase1_pq.only_if_higher);
        };

        // Extract the minimal transitions going from  Fc to F.
        std::ranges::fill(nonneg_out_edges_to_Fc, 0);
        for (auto&& v : teller.undecided_vertices ()) {
          if (not F[v]) {
            if (nrg_game.is_max (v) ^ SwapRoles) {
              nonneg_out_edges_to_Fc[v] = 0;
              for (auto&& o : nrg_game.outs (v))
                if (not F[o.second] and (SwapRoles ? o.first <= 0 : o.first >= 0))
                  nonneg_out_edges_to_Fc[v]++;
              if (nonneg_out_edges_to_Fc[v] == 0)
                add_vertex_to_phase1_pq (v);
            }
          }
          else {
            for (auto&& i : nrg_game.ins (v)) {
              if (not F[i.second] and not (nrg_game.is_max (i.second) ^ SwapRoles)) {
                // Use a weight proxy to avoid duplication.
                phase2_pq.set (i.second, weight_t::proxy (const_cast<weight_t&> (i.first)),
                               phase2_pq.only_if_higher);
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
          [this, &phase1_pq, &phase2_pq, &add_vertex_to_phase1_pq] (vertex_t v) {
            for (auto&& i : nrg_game.ins (v)) {
              if (F[i.second]) continue;

              if (nrg_game.is_max (i.second) ^ SwapRoles) { // Predecessor is Max
                if (nonneg_out_edges_to_Fc[i.second] > 0 and (SwapRoles ? i.first <= 0 : i.first >= 0)) {
                  if (--nonneg_out_edges_to_Fc[i.second] == 0)
                    add_vertex_to_phase1_pq (i.second);
                }
                else if (nonneg_out_edges_to_Fc[i.second] == 0)
                  phase1_pq.set (i.second, i.first + potential[v], phase1_pq.only_if_higher);
              }
              else { // Predecessor is Min
                weight_t w = weight_t::copy (i.first);
                w += potential[v];
                phase2_pq.set (i.second, weight_t::steal (w), phase2_pq.only_if_higher); //!! should be steal
              }

            }
          };

        while (true) {
          TICK (pot_iter);
          /* 1. If there is a Max vertex v notin F, all of whose non-negative outgoing
           * edges vv' lead to F, set, En+(v) to be the maximal w(vv') + En+(v'),
           * add v to F, and go back to 1.
           */
          log ("Phase 1.\n");
          while (not phase1_pq.empty ()) {
            TICK (pot_phase1);
            auto from = phase1_pq.top ().key;
            auto weight = std::move (phase1_pq.top ().priority);
            phase1_pq.pop ();
            assert (not F[from] and nonneg_out_edges_to_Fc[from] == 0);
            potential[from] = weight_t::steal_or_proxy (weight);
            log ("Putting " << from << " in F with pot " << potential[from] << std::endl);
            F[from] = true;
            decrease_preds (from);
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
            TICK (pot_phase2);
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
              if (not F[o.second] and (SwapRoles ? o.first <= 0 : o.first >= 0)) {
                strat[v] = o.second;
                break;
              }
        }

        // This is reused to save a bit of memory.  It will be used for player
        // min ^ SwapRoles, and it is already zero in these locations.
        auto& out_edges_in_F = nonneg_out_edges_to_Fc;

        // In addition, we compute the attractor of the newly discovered decided nodes
        while (not to_backtrack.empty ()) {
          TICK (pot_backtrack);
          auto v = to_backtrack.back ();
          to_backtrack.pop_back ();
          for (auto&& wi : nrg_game.ins (v)) {
            auto i = wi.second;
            if (not F[i])  // Already treated.
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
