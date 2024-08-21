#pragma once
#include <ranges>
#include "oink/solver.hpp"

#include "posets/downsets.hh"
#include "posets/vectors.hh"

namespace pg {

  class DownsetsSolver : public Solver {
      using vertex_t = int32_t;
      using weight_t = int16_t;
      using vector_t = posets::vectors::vector_backed<weight_t>;
      using downset_t = posets::downsets::kdtree_backed<vector_t>;
    public:
      DownsetsSolver (Oink& oink, Game& game);
      virtual ~DownsetsSolver ();
      virtual void run ();

    private:
      const Game& pgame;
      const size_t dim;
      const std::ranges::iota_view<vertex_t, vertex_t> all_verts;
      std::vector<weight_t> maxes;
      vector_t max_vec;
      std::vector<downset_t> cur;
      void init_maxes (bool swap = false);
      void init_downsets (bool swap = false);
      bool compute_cpre (bool swap = false);
  };
}
