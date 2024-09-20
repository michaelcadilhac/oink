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

#include "sepm.hpp"

namespace pg {

  SEPMSolver::SEPMSolver (Oink& oink, Game& game) :
    Solver (oink, game),
    nrg_game (this->game, logger, trace),
    n_nodes (game.nodecount ()),
    limit (0),
    strategy (game.getStrategy ()),
    weight (new weight_t[n_nodes]),
    cost (new weight_t[n_nodes]),
    count (new int[n_nodes]) {
    TAtr.resize (n_nodes);
    BAtr.resize (n_nodes);
  }

  SEPMSolver::~SEPMSolver () {
    delete[] weight;
    delete[] cost;
    delete[] count;
  }

  void SEPMSolver::run () {
    for (pos = 0; pos < n_nodes; ++pos) {
      cost[pos] = 0;
      count[pos] = 0;
      weight[pos] = nrg_game.some_outweight (pos);

      if (nrg_game.some_outweight (pos) > 0) {
        TAtr.push (pos);
        BAtr[pos] = true;
        limit += weight[pos];
      } else {
        if (nrg_game.is_min (pos)) {
          count[pos] = nrg_game.outs (pos).size ();
        }
      }
    }

    limit += 1;

    int best_succ;
    weight_t sum;

    while (TAtr.nonempty ()) {
      pos = TAtr.pop ();
      BAtr[pos] = false;
      oldcost = cost[pos];
      best_succ = -1;

      if (nrg_game.is_min (pos)) {
        for (const auto& [w, successor] : nrg_game.outs (pos)) {
          if (best_succ == -1) {
            best_succ = successor;
            count[pos] = 1;
          } else {
            if (cost[best_succ] > cost[successor]) {
              best_succ = successor;
              count[pos] = 1;
            } else if (cost[best_succ] == cost[successor]) {
              count[pos] = count[pos] + 1;
            }
          }
        }

        if (cost[best_succ] >= limit) {
          count[pos] = 0;
          cost[pos] = limit;
        } else {
          sum = cost[best_succ] + weight[pos];

          if (sum >= limit) {
            sum = limit;
          }

          if (cost[pos] < sum) {
            cost[pos] = sum;
          }
        }
      } else {
        for (const auto& [w, successor] : nrg_game.outs (pos)) {
          if (best_succ == -1) {
            best_succ = successor;
          } else {
            if (cost[best_succ] < cost[successor]) {
              best_succ = successor;
            }
          }
        }

        if (cost[best_succ] >= limit) {
          cost[pos] = limit;
          strategy[pos] = best_succ;
        } else {
          weight_t sum = cost[best_succ] + weight[pos];

          if (sum >= limit) {
            sum = limit;
          }

          if (cost[pos] < sum) {
            cost[pos] = sum;
            strategy[pos] = best_succ;
          }
        }
      }

      for (const auto& [w, predecessor] : nrg_game.ins (pos)) {
        if (!BAtr[predecessor] && (cost[predecessor] < limit) && ((cost[pos] == limit) || (cost[predecessor] < cost[pos] + weight[predecessor]))) {
          if (nrg_game.is_min (predecessor)) {
            if (cost[predecessor] >= oldcost + weight[predecessor]) {
              count[predecessor] = count[predecessor] - 1;
            }

            if (count[predecessor] <= 0) {
              TAtr.push (predecessor);
              BAtr[predecessor] = true;
            }
          } else {
            TAtr.push (predecessor);
            BAtr[predecessor] = true;
          }
        }
      }

    }

    for (pos = 0; pos < n_nodes; ++pos) {
      if (cost[pos] >= limit) {
        Solver::solve (pos, 0, strategy[pos]);
      } else {
        Solver::solve (pos, 1, strategy[pos]);
      }
    }
  }

}
