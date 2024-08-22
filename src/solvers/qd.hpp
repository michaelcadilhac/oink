/*
 * Copyright 2019 Massimo Benerecetti, Fabio Mogavero, Daniele Dell'Erba
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

#pragma once

#include "oink/solver.hpp"
#include <boost/heap/pairing_heap.hpp>

namespace pg {

  class QDSolver : public Solver {
    public:

      /****************************************************************************/
      /* Constructor and destructor                                               */
      /****************************************************************************/

      QDSolver (Oink& oink, Game& game);

      virtual ~QDSolver ();

      /****************************************************************************/

      /****************************************************************************/
      /* Main method                                                              */
      /****************************************************************************/

      // Main solver function
      virtual void run ();

      /****************************************************************************/

      /****************************************************************************/
      /* Getter method                                                              */
      /****************************************************************************/

      struct data {
          int position;
          long int bef;

          data (int pos, long int befx):
            position (pos), bef (befx)
          {}

          bool operator< (data const& rhs) const {
            return bef > rhs.bef;
          }

          data ():
            position (0), bef (0)
          {}

          void set (int pos, long int befx) {
            position = pos;
            bef = befx;
          }
      };

      /****************************************************************************/

    protected:

      /****************************************************************************/
      /* Game fields                                                              */
      /****************************************************************************/

      int* strategy;    // Strategies of the players
      bitset ingame;    // Positions whose winner has been already determined
      long int* weight; // weight of each node
      unsigned int n_nodes; // number of nodes in game

      /****************************************************************************/

      /****************************************************************************/
      /* Measure function fields                                                  */
      /****************************************************************************/

      long int* msr;    // Measure function
      long int oldmsr;  // Old measure function
      int* newsucc;     // New strategy
      int* count;       // Counter
      bitset TopBef;    // Auxiliary membership bitset for the tail queue

      boost::heap::pairing_heap<data> pq;
      typedef typename boost::heap::pairing_heap<data>::handle_type handle_t;
      handle_t* PBef;
      data pair;
      /****************************************************************************/

      /****************************************************************************/
      /* Accessory region fields                                                  */
      /****************************************************************************/

      uint pos;         // Working position
      long int bef;     // Working position
      int best_succ;    // Working position
      bitset E;         // Escape positions of the working region

      uintqueue TAtr;   // Tail queue for positions waiting for attraction
      bitset BAtr;      // Auxiliary membership bitset for the tail queue
      uintqueue TProm;  // Tail queue for positions waiting for attraction
      bitset BProm;     // Auxiliary membership bitset for the tail queue
      bitset BQset;     // Auxiliary membership bitset for the tail queue

      /****************************************************************************/

    private:

      /****************************************************************************/
      /* Accessory methods                                                        */
      /****************************************************************************/

      // Attractor function
      inline void atr ();

      // Promotion function
      inline void promo ();

      // Compute the delta set
      inline void delta ();

      // Compute the delta set
      inline void escape ();

      // Compute the delta set
      inline void nextpush (int predecessor);
      /****************************************************************************/

  };
}
