#pragma once
#include <ranges>
#include "oink/solver.hpp"

#include "posets/downsets.hh"
#include "posets/vectors.hh"

namespace pg {

  template <typename Downset>
  class DownsetsSolver : public Solver {
      using vertex_t = int32_t;
      using downset_t = Downset;
      using vector_t = typename Downset::value_type;
      using weight_t = typename vector_t::value_type;

    public:
      DownsetsSolver (Oink& oink, Game& game)  :
        Solver (oink, game),
        pgame (game),
        dim ((pgame.priority (pgame.nodecount () - 1) + 2) / 2),
        all_verts {(vertex_t) 0, (vertex_t) pgame.nodecount ()},
        max_vec (dim) {
      }

      virtual ~DownsetsSolver () {}
    private:
      void init_maxes (bool swap = false) {
        maxes.clear ();
        maxes.resize (dim);
        for (auto v : all_verts) {
          auto prio = pgame.priority (v);
          if (prio % 2 == (not swap))
            ++maxes[prio / 2];
        }
        max_vec = vector_t (maxes);
      }

      void init_downsets (bool swap = false) {
        init_maxes (swap);
        cur.reserve (pgame.nodecount ());
        for ([[maybe_unused]] auto vert : all_verts)
          cur.emplace_back (max_vec.copy ());
      }

      bool compute_cpre (bool swap = false) {
        std::vector<vector_t> save_curvert;
        bool has_changed = false;

        for (auto vert : all_verts) {
          if (not has_changed) {
            save_curvert.clear ();
            save_curvert.reserve (cur[vert].size ());
            for (const auto& vec : cur[vert])
              save_curvert.push_back (vec.copy ());
          }

          size_t nbwds = 0;
          for (auto neigh = outs (vert); *neigh != -1; ++neigh)
            nbwds += cur[*neigh].size ();

          auto out = downset_t (max_vec.copy ());
          std::vector<vector_t> bwds;
          std::vector<weight_t> bwdvec (max_vec.size ());
          bwds.reserve (nbwds);

          auto prio = pgame.priority (vert);
          for (auto neigh = outs (vert); *neigh != -1; ++neigh) {
            for (const auto& vec : cur[*neigh]) {
              vec.to_vector (bwdvec);
              // auto prio = pgame.priority (*neigh);
              if (prio % 2 == (not swap)) {
                if (bwdvec[prio / 2] >= 0)
                  bwdvec[prio / 2] = bwdvec[prio / 2] - 1;
              }
              else {
                for (int j = 0; j < (prio + 1) / 2; ++j)
                  if (bwdvec[j] >= 0)
                    bwdvec[j] = maxes[j];
              }
              bwds.push_back (vector_t (bwdvec));
            }
            if (pgame.owner (vert) == (not swap)) {
              out.intersect_with (std::move (bwds));
              bwds.clear ();
            }
          }
          if (pgame.owner (vert) == swap)
            out = downset_t (std::move (bwds));

          if (not has_changed) {
            // Check inclusion of cur[vert] in out
            for (const auto& vec : cur[vert])
              if (not out.contains (vec)) {
                // not included: there will be a change after intersection
                has_changed = true;
                break;
              }
          }
          if (has_changed) { // small optimization: has_changed is false here if the
            // previous for loop found that cur[vert] is included in
            // out.
            // TODO document
            out.intersect_with (std::move (cur[vert]));
            cur[vert] = std::move (out);
            //cur[vert].intersect_with (downset_t (std::move (out)));
          }
        }

        return has_changed;
      }
    public:

      virtual void run () {
        init_downsets ();

        while (compute_cpre ()) {
          /*for (auto vert : all_verts) {
           std::cout << pv (vert) << cur[vert] << "\n";
           }
           std::cout << "-----------------\n";*/
        }

        vector_t all_zeroes { std::vector<weight_t> (dim) };

        auto cmp = [this] (const vector_t& l, const vector_t& r, ssize_t idx) {
          for (ssize_t j = dim - 1; j >= idx; --j)
            if (l[j] != r[j])
              return l[j] - r[j];
          return 0;
        };

        for (auto vert : all_verts) {
          if (pgame.isSolved (vert)) { /*std::cout << pv (vert) << "already solved.\n";*/ continue; }
          if (cur[vert].contains (all_zeroes)) {
            // even wins
            if (pgame.owner (vert) == 0) {
              size_t idx = pgame.priority (vert) / 2;
              const vector_t* min = nullptr;
              vertex_t strat = -1;
              for (auto neigh = outs (vert); *neigh != -1; ++neigh) {
                for (const auto& v : cur[*neigh])
                  if (v.partial_order (all_zeroes).geq () and
                      (min == nullptr or cmp (*min, v, idx) < 0)) {
                    min = &v;
                    strat = *neigh;
                  }
              }
              //std::cout << pv (vert) << " wins going " << strat << "\n";
              Solver::solve (vert, 0, strat);
            }
            else {
              //std::cout << pv (vert) << " loses\n";
              Solver::solve (vert, 0, -1);
            }
          }
        }
        //std::cout << "Dual game.\n";
        // second round!
        /*cur.clear ();
         init_downsets (true);

         while (compute_cpre (true)) {
         /*for (auto vert : all_verts)
         std::cout << pv (vert) << cur[vert] << "\n";
         std::cout << "-----------------\n";*/
        //}

        /*        for (auto vert : all_verts) {
          if (pgame.isSolved (vert)) continue;
          if (cur[vert].contains (all_zeroes)) {
            // odd wins
            if (pgame.owner (vert) == 1) {
              size_t idx = (pgame.priority (vert)) / 2;
              const vector_t* min = nullptr;
              vertex_t strat = -1;
              for (auto neigh = outs (vert); *neigh != -1; ++neigh) {
                for (const auto& v : cur[*neigh])
                  if (v.partial_order (all_zeroes).geq () and
                      (min == nullptr or cmp (*min, v, idx) < 0)) {
                    min = &v;
                    strat = *neigh;
                  }
              }
              //std::cout << pv (vert) << " wins going " << strat << "\n";
              Solver::solve (vert, 1, strat);
            }
            else {
              //std::cout << pv (vert) << " loses\n";
              Solver::solve (vert, 1, -1);
            }
          }
         } */
      }
    private:
      const Game& pgame;
      const size_t dim;
      const std::ranges::iota_view<vertex_t, vertex_t> all_verts;
      std::vector<weight_t> maxes;
      vector_t max_vec;
      std::vector<downset_t> cur;
  };
}
