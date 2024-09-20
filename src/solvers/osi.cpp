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

#include "osi.hpp"

namespace pg {

  OSISolver::OSISolver (Oink& oink, Game& game) :
    Solver (oink, game),
    nrg_game (this->game, logger, trace),
    n_nodes (game.nodecount ()),
    strategy (game.getStrategy ()),
    weight (new weight_t[n_nodes]),
    measure (new weight_t[n_nodes]),
    update (new weight_t[n_nodes]) {
    Bq.resize (n_nodes);
    Bp.resize (n_nodes);
    Bpp.resize (n_nodes);
    Gq.resize (n_nodes);
    Gp.resize (n_nodes);
    Valuate.resize (n_nodes);
    Initial.resize (n_nodes);
    Top.resize (n_nodes);
  }

  OSISolver::~OSISolver () {
    delete[] weight;
    delete[] measure;
    delete[] update;
  }

  _INLINE_ void OSISolver::init () {
    improve = true;

    for (pos = 0; pos < n_nodes; ++pos) {
      weight[pos] = nrg_game.some_outweight (pos);
      update[pos] = 0;
      Valuate[pos] = true;
      Initial[pos] = true;
      Top[pos] = false;
      measure[pos] = 0;
    }

    for (pos = 0; pos < n_nodes; ++pos) {
      if (nrg_game.is_min (pos)) {
        Initial[pos] = false;
        best_succ = -1;

        for (const auto& [w, successor] : nrg_game.outs (pos)) {
          if (best_succ == -1 || weight[best_succ] > weight[successor]) {
            best_succ = successor;
          }
        }

        measure[pos] = weight[best_succ];
      }
    }
  }

  _INLINE_ void OSISolver::innesco () {
    for (pos = 0; pos < n_nodes; ++pos) {
      if (!Top[pos] && nrg_game.is_max (pos) && Initial[pos]) {
        if (measure[pos] > 0) {
          Initial[pos] = false;
        } else {
          for (const auto& [w, successor] : nrg_game.outs (pos)) {
            if (Top[successor] || measure[pos] <= measure[successor] + weight[successor]) {
              Initial[pos] = false;
            }
          }
        }
      }
    }

    for (uint i = Initial.find_first (); i < (uint) n_nodes; i = Initial.find_next (i)) {
      update[i] = 0;
      Valuate[i] = false;

      for (const auto& [w, predecesor] : nrg_game.ins (i)) {
        if (Valuate[predecesor] && !Bp[predecesor]) {
          Bq.push (predecesor);
          Bp[predecesor] = true;
        }
      }
    }
  }

  _INLINE_ bool OSISolver::caseA () {
    bool change = false;

    while (Bq.nonempty ()) {
      int i = Bq.pop ();
      Bp[i] = false;

      if (Valuate[i]) {
        Bpp[i] = true;
        check = true;
        best_succ = -1;
        weight_t best_update = -1;

        for (const auto& [w, successor] : nrg_game.outs (i)) {
          if (Valuate[successor]) {
            check = false;
          } else {
            if (!Top[successor] && update[successor] == 0 && (measure[successor] + weight[successor] == measure[i])) {
              check = false;
              update[i] = 0;
              Valuate[i] = false;
              Bpp[i] = false;
              change = true;

              for (const auto& [w, predecesor] : nrg_game.ins (i)) {
                if (Valuate[predecesor] && !Gp[predecesor]) {
                  Gq.push (predecesor);
                  Gp[predecesor] = true;
                }
              }

              break;
            } else {
              if (best_succ == -1 || Top[best_succ] || (!Top[successor] && (best_update > update[successor] + measure[successor] + weight[successor]))) {
                best_update = update[successor] + measure[successor] + weight[successor];
                best_succ = successor;
              }
            }
          }
        }

        if (check && best_succ > -1) {
          if (Top[best_succ]) {
            Top[i] = true;
            improve = true;
          } else {
            if (best_update - measure[i] > 0) {
              update[i] = best_update - measure[i];
              improve = true;
            }
          }

          Valuate[i] = false;
          Bpp[i] = false;
          change = true;

          for (const auto& [w, predecesor] : nrg_game.ins (i)) {
            if (Valuate[predecesor] && !Gp[predecesor]) {
              Gq.push (predecesor);
              Gp[predecesor] = true;
            }
          }
        }
      }
    }

    return change;
  }

  _INLINE_ bool OSISolver::caseC () {
    bool change = false;

    while (Gq.nonempty () && !change) {
      int i = Gq.pop ();
      Gp[i] = false;

      if (Valuate[i]) {
        check = true;
        best_succ = -1;
        weight_t best_update = -1;

        for (const auto& [w, successor] : nrg_game.outs (i)) {
          if (check && (Top[successor] || (measure[i] <= measure[successor] + weight[successor]))) {
            if (Valuate[successor]) {
              check = false;
              break;
            } else {
              if (best_succ == -1 || Top[successor] || (!Top[best_succ] && (best_update < update[successor] + measure[successor] + weight[successor]))) {
                best_update = update[successor] + measure[successor] + weight[successor];
                best_succ = successor;
              }
            }
          }
        }

        if (check && best_succ > -1) {
          if (Top[best_succ]) {
            Top[i] = true;
            improve = true;
          } else {
            if (best_update - measure[i] > 0) {
              update[i] = best_update - measure[i];
              improve = true;
            }
          }

          Valuate[i] = false;
          change = true;

          for (const auto& [w, predecesor] : nrg_game.ins (i)) {
            if (Valuate[predecesor] && !Bp[predecesor]) {
              Bq.push (predecesor);
              Bp[predecesor] = true;
            }
          }

          break;
        }
      }
    }

    return change;
  }

  _INLINE_ bool OSISolver::caseD () {
    bool change = false;
    int best_min = -1;
    weight_t best_update = -1;
    best_succ = -1;

    for (uint i = Bpp.find_first (); i < (uint) n_nodes; i = Bpp.find_next (i)) {
      if (Valuate[i]) {
        for (const auto& [w, successor] : nrg_game.outs (i)) {
          if (!Valuate[successor]) {
            if (best_succ == -1 || Top[best_succ] || (!Top[successor] && (best_update > update[successor] + measure[successor] + weight[successor] - measure[i]))) {
              best_min = i;
              best_succ = successor;
              best_update = update[successor] + measure[successor] + weight[successor] - measure[i];
            }
          }
        }
      } else {
        Bpp[i] = false;
      }
    }

    if (best_succ > -1) {
      if (Top[best_succ]) {
        Top[best_min] = true;
        improve = true;
      } else {
        if (best_update > 0) {
          update[best_min] = best_update;
          improve = true;
        }
      }

      Valuate[best_min] = false;
      Bpp[best_min] = false;
      change = true;

      for (const auto& [w, predecesor] : nrg_game.ins (best_min)) {
        if (Valuate[predecesor] && !Gp[predecesor]) {
          Gq.push (predecesor);
          Gp[predecesor] = true;
        }
      }
    }

    return change;
  }

  void OSISolver::run () {
    init ();

    while (improve) {
      innesco ();

      improve = false;
      bool check_caseA = true;
      bool check_caseC = true;
      bool check_caseD = true;

      do {
        do {
          do {
            check_caseA = caseA ();
          } while (check_caseA);

          check_caseC = caseC ();
        } while (check_caseC);

        check_caseD = caseD ();
      } while (check_caseD);

      if (Valuate.any ()) {
        improve = true;

        for (uint i = Valuate.find_first (); i < (uint) n_nodes; i = Valuate.find_next (i)) {
          Top[i] = true;
        }
      }

      if (improve) {
        for (pos = 0; pos < n_nodes; ++pos) {
          measure[pos] = measure[pos] + update[pos];
          update[pos] = 0;

          if (!Top[pos]) {
            Valuate[pos] = true;
          } else {
            Valuate[pos] = false;
          }
        }
      }
    }

    for (pos = 0; pos < n_nodes; ++pos) {
      if (Top[pos]) {
        Solver::solve (pos, 0, strategy[pos]);
      } else {
        Solver::solve (pos, 1, strategy[pos]);
      }
    }
  }

}
