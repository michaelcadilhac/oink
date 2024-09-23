#pragma once

#include "solvers/potential/stats.hpp"

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
        F.assign (nrg_game.size (), false);
        for (auto& p : potential)
          p = SwapRoles ? nrg_game.get_minus_infty () : nrg_game.get_infty ();

        for (auto&& v : teller.undecided_vertices ()) {
          if (not F[v]) {
            if (SwapRoles ^ nrg_game.is_max (v))
              F[v] = std::ranges::all_of (nrg_game.outs (v), [this] (auto& x) { return SwapRoles ? x.first > 0 : x.first < 0; });
            else
              F[v] = std::ranges::any_of (nrg_game.outs (v), [this] (auto& x) { return SwapRoles ? x.first > 0 : x.first < 0; });
          }
          if (F[v]) {
            potential[v] = zero_number (*nrg_game.get_infty ());
            log ("Putting " << v << " in F, with pot 0.\n");
          }
        }

        std::queue<vertex_t> phase1_queue;
        using wv_t = std::pair<weight_t, vertex_t>;

        auto gt_or_lt = [] (const wv_t& l, const wv_t& r) {
          return SwapRoles ? l < r : l > r;
        };

        std::priority_queue<wv_t, std::vector<wv_t>, decltype (gt_or_lt)> phase2_pq (gt_or_lt);

        for (auto&& v : teller.undecided_vertices ()) {
          if (not F[v]) continue;
          for (auto&& i : nrg_game.ins (v)) {
            if (not F[i.second] and not (nrg_game.is_max (i.second) ^ SwapRoles)) {
               // Use a weight proxy to avoid duplication.
              phase2_pq.push (std::make_pair (weight_t::proxy (const_cast<weight_t&> (i.first)), i.second));
            }
          }
        }

        for (auto&& v : SwapRoles ? teller.undecided_min_vertices () : teller.undecided_max_vertices ()) {
          if (F[v]) continue;
          nonneg_out_edges_to_Fc[v] = 0;
          for (auto&& o : nrg_game.outs (v))
            if (not F[o.second] and (SwapRoles ? o.first <= 0 : o.first >= 0))
              nonneg_out_edges_to_Fc[v]++;
          if (nonneg_out_edges_to_Fc[v] == 0)
            phase1_queue.push (v);
        }

        /* For each vertex from Max, we keep track of the number of edges that
         * go *to* F.  When a vertex is added to F, we simply look at its
         * predecessors, and decrease their count.  This allows detecting
         * efficiently which vertices should be added to F (i.e., when their
         * counter is zero).
         */
        auto decrease_preds =
          [this, &phase1_queue, &phase2_pq] (vertex_t v) {
          for (auto&& i : nrg_game.ins (v)) {
            if (not (nrg_game.is_max (i.second) ^ SwapRoles) and not F[i.second]) {
              weight_t w = weight_t::copy (i.first);
              w += potential[v];
              phase2_pq.push (std::make_pair (weight_t::steal (w), i.second)); //!! should be steal
            }
            if (nonneg_out_edges_to_Fc[i.second] and (SwapRoles ? i.first <= 0 : i.first >= 0)) {
              assert ((SwapRoles ^ nrg_game.is_max (i.second)) and not F[i.second]);
              if (--nonneg_out_edges_to_Fc[i.second] == 0)
                phase1_queue.push (i.second);
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
              if (SwapRoles ? o.first > 0 : o.first < 0) continue;
              assert (F[o.second]);
              if (SwapRoles)
                set_if_plus_smaller (*potential[v], *o.first, *potential[o.second]);
              else
                set_if_plus_larger (*potential[v], *o.first, *potential[o.second]);
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
            auto xsecond = phase2_pq.top ().second;
            auto xfirst = weight_t::steal (const_cast<wv_t&> (phase2_pq.top ()).first);
            phase2_pq.pop ();
            if (F[xsecond]) continue;
            potential[xsecond] = weight_t::steal_or_copy (xfirst);
            F[xsecond] = true;
            log ("Putting " << xsecond << " in F with pot " << potential[xsecond] << std::endl);
            decrease_preds (xsecond);
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

        // In addition, backtrack the item
        while (not to_backtrack.empty ()) {
          C (pot_backtrack);
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
              bool all_out_decided = true;
              for (auto&& io : nrg_game.outs (i))
                if (F[io.second]) {
                  all_out_decided = false;
                  break;
                }
              if (all_out_decided) {
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
