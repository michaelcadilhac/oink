#pragma once

ADD_TO_STATS (pot_compute);
ADD_TO_STATS (pot_iter);
ADD_TO_STATS (pot_phase1);
ADD_TO_STATS (pot_phase2);
ADD_TO_STATS (pot_backtrack);

namespace potential_computers {
  template <typename T>
  struct gt_or_lt {
      inline static bool swap = false;
      bool operator()(const T& l, const T& r) const { return swap ? l < r : l > r; }
  };

  template <bool SwapRoles, typename W>
  class potential_fvi_swap : public potential_computer<W> {
    public:
      std::vector<bool> F;
      std::vector<vertex_t> strat;
      std::vector<size_t> nonneg_out_edges_to_Fc; // Nonzero entries will only
                                                  // be for (max ^ SwapRoles)
                                                  // vertices.

      potential_fvi_swap (const energy_game<W>& game, logger_t& logger, int trace) :
        potential_computer<W> (game, logger, trace), F (game.size ()),
        strat (game.size (), -1), nonneg_out_edges_to_Fc (game.size (), 0) {
        for (auto& p : this->potential)
          p = game.get_infty ();
      }

      std::optional<vertex_t> strategy_for (vertex_t v) {
        auto& game = this->game;

        if ((SwapRoles ^ game.is_max (v)) and strat[v] != -1)
          return strat[v];

        if ((SwapRoles ^ game.is_min (v)) and
            (SwapRoles ? game.get_potential ()[v] > game.get_minus_infty ()
                       : game.get_potential ()[v] < game.get_infty ()))
          for (auto&& o : game.outs (v))
            if (SwapRoles ? (game.get_potential ()[o.second] > game.get_minus_infty () and o.first >= 0)
                : (game.get_potential ()[o.second] < game.get_infty () and o.first <= 0))
              return o.second;
        return std::nullopt;
      }

      std::vector<vertex_t> to_backtrack;
      void compute () {
        auto& game = this->game;
        auto& potential = this->potential;

        C (pot_compute);

        /*
         We start by determining in linear time O(m) the set N of vertices from
         which Min can force to immediately visit an edge of negative weight;
         these have En+-value 0. We will successively update a set F containing
         the set of vertices over which En+ is currently known. We initialise
         this set F to N. Note that all remaining Min vertices have only
         non-negative outgoing edges, and all remaining Max vertices have (at
         least) a non-negative outgoing edge.
         */
        log ("Initialization of F\n");
        F.assign (game.size (), false);
        for (auto& p : potential)
          p = SwapRoles ? game.get_minus_infty () : game.get_infty ();

        for (auto&& v : game.undecided_vertices ()) {
          if (not F[v]) {
            if (SwapRoles ^ game.is_max (v))
              F[v] = std::ranges::all_of (game.outs (v), [this] (auto& x) { return SwapRoles ? x.first > 0 : x.first < 0; });
            else
              F[v] = std::ranges::any_of (game.outs (v), [this] (auto& x) { return SwapRoles ? x.first > 0 : x.first < 0; });
          }
          if (F[v]) {
            potential[v] = zero_weight (*game.get_infty ());
            log ("Putting " << v << " in F, with pot 0.\n");
          }
        }

        std::queue<vertex_t> phase1_queue;
        using wv_t = std::pair<W, vertex_t>;

        gt_or_lt<wv_t>::swap = SwapRoles;
        std::priority_queue<wv_t, std::vector<wv_t>,
                            gt_or_lt<wv_t>> phase2_pq;

        for (auto&& v : game.undecided_vertices ()) {
          if (not F[v]) continue;
          for (auto&& i : game.ins (v)) {
            if (not F[i.second] and not (game.is_max (i.second) ^ SwapRoles)) {
              phase2_pq.push (std::make_pair (W::proxy (const_cast<W&> (i.first)), i.second)); // !!! should be proxy
            }
          }
        }

        for (auto&& v : SwapRoles ? game.undecided_min_vertices () : game.undecided_max_vertices ()) {
          if (F[v]) continue;
          nonneg_out_edges_to_Fc[v] = 0;
          for (auto&& o : game.outs (v))
            if (not F[o.second] and (SwapRoles ? o.first <= 0 : o.first >= 0))
              nonneg_out_edges_to_Fc[v]++;
          if (nonneg_out_edges_to_Fc[v] == 0)
            phase1_queue.push (v);
        }

        // Pour chaque sommet max, tu retiens le nombre d'arretes qui vont vers
        // F. Quand tu rajoutes un sommet dans F, tu as juste à regarder les
        // prédécesseurs qui sont des sommets Max, et à enlever 1 au
        // décompte. Ca permet de détecter efficavement les sommets Max qu'il
        // faut ajouter à F (quand leur décompte tombe à zéro)
        auto decrease_preds =
          [this, &game, &potential, &phase1_queue, &phase2_pq] (vertex_t v) {
          for (auto&& i : game.ins (v)) {
            if (not (game.is_max (i.second) ^ SwapRoles) and not F[i.second]) {
              W w = W::copy (i.first);
              w += potential[v];
              phase2_pq.push (std::make_pair (W::steal (w), i.second)); //!! should be steal
            }
            if (nonneg_out_edges_to_Fc[i.second] and (SwapRoles ? i.first <= 0 : i.first >= 0)) {
              assert ((SwapRoles ^ game.is_max (i.second)) and not F[i.second]);
              if (--nonneg_out_edges_to_Fc[i.second] == 0)
                phase1_queue.push (i.second);
            }
          }
        };
        /*
         We then iterate the two following steps illustrated in Figure 4. (A
         complexity analysis is given below.)
         */
        while (true) {
          C (pot_iter);
          /*
           1. If there is a Max vertex v /∈ F all of whose non-negative outgoing
           edges vv′ lead to F, set En+(v) to be the maximal w(vv′) + En+(v′),
           add v to F, and go back to 1.
           */
          log ("Phase 1.\n");
          while (not phase1_queue.empty ()) {
            C (pot_phase1);
            vertex_t v = phase1_queue.front ();
            phase1_queue.pop ();
            assert (not F[v] and nonneg_out_edges_to_Fc[v] == 0);
            potential[v] = SwapRoles ? game.get_infty () : game.get_minus_infty ();
            for (auto&& o : game.outs (v)) {
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

          /*
           2. Otherwise, let vv′ be an edge from VMin \ F to F (it is
           necessarily positive) minimising w(vv′) + En+(v′); set En+(v) =
           w(vv′) + En+(v′), add v to F and go back to 1. If there is no such
           edge, terminate.
           For step 2, one should store, for each v ∈ VMax \ F, the edge towards
           F minimising w(vv′) + En+(v′) in a priority queue.
           */
          log ("Phase 2.\n");
          bool change = false;
          while (not phase2_pq.empty ()) {
            C (pot_phase2);
            auto xsecond = phase2_pq.top ().second;
            auto xfirst = W::steal (const_cast<wv_t&> (phase2_pq.top ()).first);
            phase2_pq.pop ();
            if (F[xsecond]) continue;
            potential[xsecond] = W::steal_or_copy (xfirst);
            F[xsecond] = true;
            log ("Putting " << xsecond << " in F with pot " << potential[xsecond] << std::endl);
            decrease_preds (xsecond);
            change = true;
            break;
          }
          if (not change)
            break;
        }
        /*
         After the iteration has terminated, there remains to deal with Fc,
         which is the set of vertices from which Max can ensure to visit only
         non-negative edges forever. Since the arena is assumed to be simple (no
         simple cycle has weight zero) it holds that En+ is ∞ over Fc, and we
         are done. */
        to_backtrack.reserve (game.undecided_vertices ().size ());
        to_backtrack.clear ();
        for (auto&& v : game.undecided_vertices ()) {
          if (F[v]) continue;
          to_backtrack.push_back (v);
          if ((SwapRoles ^ game.is_max (v)) and strat[v] == -1) // Find a strategy
            for (auto&& o : game.outs (v))
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
          for (auto&& wi : game.ins (v)) {
            auto i = wi.second;
            if (not F[i])  // Already treated.
              continue;
            if (SwapRoles ^ game.is_max (i)) {
              F[i] = false;
              potential[i] = SwapRoles ? game.get_minus_infty () : game.get_infty ();
              strat[i] = v;
              to_backtrack.push_back (i);
            }
            else {
              bool all_out_decided = true;
              for (auto&& io : game.outs (i))
                if (F[io.second]) {
                  all_out_decided = false;
                  break;
                }
              if (all_out_decided) {
                F[i] = false;
                potential[i] = SwapRoles ? game.get_minus_infty () : game.get_infty ();
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

  template <typename W>
  using potential_fvi = potential_fvi_swap<false, W>;

  template <typename W>
  class potential_fvi_alt : public potential_computer<W> {
    private:
      potential_fvi_swap<false, W> fvi;
      potential_fvi_swap<true, W> fvi_swap;
      bool swap = true;
    public:
      potential_fvi_alt (const energy_game<W>& game, logger_t& logger, int trace) :
        potential_computer<W> (game, logger, trace),
        fvi (game, logger, trace), fvi_swap (game, logger, trace) {}


      std::optional<vertex_t> strategy_for (vertex_t v) {
        if (this->game.is_min (v))
          return fvi_swap.strategy_for (v);
        else
          return fvi.strategy_for (v);
      }

      void compute () {
        swap ^= true;
        if (swap)
          fvi_swap.compute ();
        else
          fvi.compute ();

        // If it's all zeroes, then do one more round of the other fvi and be done.
        auto&& pot = get_potential ();
        bool no_change = true;
        for (auto& v : this->game.undecided_vertices ())
          if (pot[v] != 0) {
            no_change = false;
            break;
          }
        if (not no_change)
          return;
        swap ^= true;
        if (swap)
          fvi_swap.compute ();
        else
          fvi.compute ();
      }

      virtual const potential_computer<W>::potential_t& get_potential () const {
        if (swap)
          return fvi_swap.get_potential ();
        else
          return fvi.get_potential ();
      }
  };
}
