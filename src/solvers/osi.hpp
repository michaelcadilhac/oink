/*
 * Copyright 2017-2018 Massimo Benerecetti, Fabio Mogavero, Daniele Dell'Erba
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

#ifndef OSI_HPP
#define OSI_HPP

#include <boost/dynamic_bitset.hpp>

#include "oink/solver.hpp"
#include "energy_game.hpp"

namespace pg {

#define _INLINE_ __attribute__ ((always_inline))

  typedef boost::dynamic_bitset<unsigned long long> boost_bitset;

  class OSISolver : public Solver {
      using weight_t = gmp_weight_t;
    public:
      OSISolver (Oink& oink, Game& game);

      virtual ~OSISolver ();
      virtual void run ();

    protected:
      energy_game<weight_t> nrg_game;

      size_t n_nodes;
      int* strategy;
      int pos;
      uintqueue Bq;
      boost_bitset Bp;
      boost_bitset Bpp;
      uintqueue Gq;
      boost_bitset Gp;
      weight_t* weight;
      weight_t* measure;
      weight_t* update;
      boost_bitset Valuate;
      boost_bitset Initial;
      boost_bitset Top;
      bool improve;
      bool check;
      int best_succ;

    private:

      inline void init ();
      inline void innesco ();
      inline bool caseA ();
      inline bool caseB ();
      inline bool caseC ();
      inline bool caseD ();

  };

}

#endif
