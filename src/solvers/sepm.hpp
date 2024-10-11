/*
 * Copyright 2020 Massimo Benerecetti, Fabio Mogavero, Daniele Dell'Erba
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef SEPM_HPP
#define SEPM_HPP

#include <boost/dynamic_bitset.hpp>

#include "oink/solver.hpp"
#include "oink/uintqueue.hpp"
#include "energy_game.hpp"

namespace pg {

#define _INLINE_ __attribute__ ((always_inline))

  typedef boost::dynamic_bitset<unsigned long long> boost_bitset;

  class SEPMSolver : public Solver {
      using weight_t = gmp_weight_t;
    public:

      SEPMSolver (Oink& oink, Game& game);

      virtual ~SEPMSolver ();
      virtual void run ();

    protected:
      energy_game<weight_t> nrg_game;

      size_t n_nodes;
      weight_t limit;
      int* strategy;
      size_t pos;
      uintqueue TAtr;
      boost_bitset BAtr;
      weight_t* weight;
      weight_t* cost;
      weight_t oldcost;
      int* count;

  };

}

#endif
