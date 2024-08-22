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

#define _INLINE_ __attribute__ ((always_inline))

#include "qd.hpp"

namespace pg {

  QDSolver::QDSolver (Oink& oink, Game& game) :
    Solver (oink, game),
    strategy (game.getStrategy ()),
    n_nodes (game.nodecount ()) {

    ingame.resize (n_nodes);

    weight = new long int [n_nodes];
    msr = new long int [n_nodes];
    newsucc = new int[n_nodes];
    count = new int[n_nodes];

    E.resize (n_nodes);

    PBef = new handle_t[n_nodes];

    TAtr.resize (n_nodes);
    BAtr.resize (n_nodes);
    TProm.resize (n_nodes);
    BProm.resize (n_nodes);
    TopBef.resize (n_nodes);
    BQset.resize (n_nodes);
  }

  QDSolver::~QDSolver () {
    delete[] weight;
    delete[] msr;
    delete[] newsucc;
    delete[] count;
    delete[] PBef;
  }

  _INLINE_ void QDSolver::atr () {
    while (TAtr.nonempty ()) {
      pos = TAtr.pop ();
      BAtr[pos] = false;
      oldmsr = msr[pos];
      best_succ = -1;

      if (game.owner (pos) == 1) {
        for (int successor : game.outvec (pos)) {
          if (best_succ == -1 || (msr[best_succ] > msr[successor])) {
            best_succ = successor;
            count[pos] = 1;
          } else if (msr[best_succ] == msr[successor]) {
            count[pos] = count[pos] + 1;
          }
        }
      } else {
        best_succ = newsucc[pos];
        strategy[pos] = best_succ;
      }

      if (ingame[best_succ]) {
        msr[pos] = msr[best_succ] + weight[pos];
      } else {
        ingame[pos] = false;
        msr[pos] = LONG_MAX;
      }

      for (auto ppredecessor = ins (pos); * ppredecessor != -1; ++ppredecessor) {
        auto predecessor = * ppredecessor;

        if (ingame[predecessor] && (!ingame[pos] || msr[predecessor] < msr[pos] + weight[predecessor])) {
          if (game.owner (predecessor) == 1) {
            if (msr[predecessor] == 0) {
              if (!BAtr[predecessor]) {
                if (0 >= oldmsr + weight[predecessor]) {
                  count[predecessor] = count[predecessor] - 1;
                }

                if (count[predecessor] <= 0) {
                  BAtr[predecessor] = true;
                }
              }
            } else {
              if (!BProm[predecessor]) {
                if (msr[predecessor] >= oldmsr + weight[predecessor]) {
                  count[predecessor] = count[predecessor] - 1;
                }

                if (count[predecessor] <= 0) {
                  TProm.push (predecessor);
                  BProm[predecessor] = true;
                }
              }
            }
          } else {
            if (msr[predecessor] == 0) {
              if (!BAtr[predecessor]) {
                BAtr[predecessor] = true;
                newsucc[predecessor] = pos;
              } else {
                if (msr[newsucc[predecessor]] < msr[pos]) {
                  newsucc[predecessor] = pos;
                }
              }
            } else {
              if (!BProm[predecessor]) {
                TProm.push (predecessor);
                BProm[predecessor] = true;
                newsucc[predecessor] = pos;
              } else {
                if (msr[newsucc[predecessor]] < msr[pos]) {
                  newsucc[predecessor] = pos;
                }
              }
            }
          }
        }
      }
    }

    for (pos = BAtr.find_first (); pos < (uint) n_nodes; pos = BAtr.find_next (pos)) {
      TAtr.push (pos);
    }
  }

  _INLINE_ void QDSolver::delta () {
    BQset = BProm;

    while (TProm.nonempty ()) {
      pos = TProm.pop ();

      for (auto ppredecessor = ins (pos); * ppredecessor != -1; ++ppredecessor) {
        auto predecessor = * ppredecessor;

        if (ingame[predecessor] && !BQset[predecessor] && (msr[predecessor] > 0)) {
          if (game.owner (predecessor) == 0) {
            if (strategy[predecessor] == (int) pos) {
              if (!BProm[predecessor]) {
                TProm.push (predecessor);
              }

              BQset[predecessor] = true;
            }
          } else {
            if (msr[predecessor] >= msr[pos] + weight[predecessor]) {
              count[predecessor] = count[predecessor] - 1;
            }

            if (count[predecessor] <= 0) {
              if (!BProm[predecessor]) {
                TProm.push (predecessor);
              }

              BQset[predecessor] = true;
            }
          }
        }
      }
    }

    BProm.reset ();
  }

  _INLINE_ void QDSolver::escape () {
    TopBef.reset ();

    for (pos = BQset.find_first (); pos < (uint) n_nodes; pos = BQset.find_next (pos)) {
      best_succ = -1;

      if (game.owner (pos) == 0) {
        for (int successor : game.outvec (pos)) {
          if (BQset[successor]) {
            if ((strategy[pos] == successor) || (msr[pos] < msr[successor] + weight[pos])) {
              count[pos] = count[pos] + 1;
            }
          } else {
            if (count[pos] == 0 && (best_succ == -1 || !ingame[successor] || msr[best_succ] < msr[successor])) {
              best_succ = successor;
            }
          }
        }

        if (count[pos] == 0) {
          if (ingame[best_succ]) {
            pair.set (pos, msr[best_succ] + weight[pos] - msr[pos]);
            PBef[pos] = pq.push (pair);
          } else {
            TopBef[pos] = true;;
          }

          E[pos] = true;
        }
      } else {
        for (int successor : game.outvec (pos)) {
          if (!BQset[successor]) {
            if (best_succ == -1 || (ingame[successor] && msr[best_succ] > msr[successor])) {
              best_succ = successor;
            }

            E[pos] = true;
          }
        }

        if (best_succ != -1) {
          if (ingame[best_succ]) {
            pair.set (pos, msr[best_succ] + weight[pos] - msr[pos]);
            PBef[pos] = pq.push (pair);
          } else {
            TopBef[pos] = true;;
          }
        }
      }
    }
  }

  _INLINE_ void QDSolver::nextpush (int predecessor) {
    if (game.owner (predecessor) == 0) {
      if (msr[predecessor] == 0) {
        if (!BAtr[predecessor]) {
          TAtr.push (predecessor);
          BAtr[predecessor] = true;
          newsucc[predecessor] = pos;
        } else {
          if (msr[newsucc[predecessor]] < msr[pos]) {
            newsucc[predecessor] = pos;
          }
        }
      } else {
        if (!BProm[predecessor]) {
          TProm.push (predecessor);
          BProm[predecessor] = true;
          newsucc[predecessor] = pos;
        } else {
          if (msr[newsucc[predecessor]] < msr[pos]) {
            newsucc[predecessor] = pos;
          }
        }
      }
    } else {
      if (msr[predecessor] == 0) {
        if (!BAtr[predecessor]) {
          if (oldmsr + weight[predecessor] <= 0) {
            count[predecessor] = count[predecessor] - 1;
          }

          if (count[predecessor] <= 0) {
            TAtr.push (predecessor);
            BAtr[predecessor] = true;
          }
        }
      }
    }
  }

  _INLINE_ void QDSolver::promo () {
    delta ();
    escape ();

    while (!pq.empty ()) {
      pair = pq.top ();
      pq.pop ();
      bef = pair.bef;
      pos = pair.position;
      E[pos] = false;
      oldmsr = msr[pos];
      msr[pos] = msr[pos] + bef;
      BQset[pos] = false;

      if (game.owner (pos) == 1) {
        for (int successor : game.outvec (pos)) {
          if (ingame[successor] && !BQset[successor] && (msr[pos] >= msr[successor] + weight[pos])) {
            count[pos] = count[pos] + 1;
          }
        }
      } else {
        strategy[pos] = newsucc[pos];
      }

      for (auto ppredecessor = ins (pos); * ppredecessor != -1; ++ppredecessor) {
        auto predecessor = * ppredecessor;

        if (predecessor != (int) pos && ingame[predecessor]) {
          if (BQset[predecessor]) {
            if (game.owner (predecessor) == 1) {
              bef = msr[pos] + weight[predecessor] - msr[predecessor];
              pair.set (predecessor, bef);

              if (!TopBef[predecessor] && E[predecessor]) {
                if (( * PBef[predecessor]).bef > bef) {
                  pq.increase (PBef[predecessor], pair);
                }
              } else {
                E[predecessor] = true;
                PBef[predecessor] = pq.push (pair);
                TopBef[predecessor] = false;
              }
            } else {
              if ((strategy[predecessor] == (int) pos) || (msr[predecessor] < oldmsr + weight[predecessor])) {
                count[predecessor] = count[predecessor] - 1;
              }

              if (!E[predecessor] && count[predecessor] == 0) {
                best_succ = -1;

                for (int successor : game.outvec (predecessor)) {
                  if (!BQset[successor]) {
                    if (game.owner (predecessor) == 0) {
                      if (best_succ == -1 || msr[best_succ] < msr[successor]) {
                        best_succ = successor;
                        newsucc[predecessor] = best_succ;
                      }
                    } else {
                      if (best_succ == -1 || msr[best_succ] > msr[successor]) {
                        best_succ = successor;
                      }
                    }
                  }
                }

                if (ingame[best_succ]) {
                  pair.set (predecessor, msr[best_succ] + weight[predecessor] - msr[predecessor]);
                  PBef[predecessor] = pq.push (pair);
                  E[predecessor] = true;
                } else {
                  BQset[predecessor] = true;
                  TopBef[predecessor] = true;;
                }
              }
            }
          } else {
            if (msr[predecessor] < msr[pos] + weight[predecessor]) {
              nextpush (predecessor);
            }
          }
        }
      }
    }

    if (BQset.any ()) {
      ingame -= BQset;

      for (pos = BQset.find_first (); pos < (uint) n_nodes; pos = BQset.find_next (pos)) {
        BQset[pos] = false;
        oldmsr = msr[pos];
        msr[pos] = LONG_MAX;

        for (auto ppredecessor = ins (pos); * ppredecessor != -1; ++ppredecessor) {
          auto predecessor = * ppredecessor;

          if (ingame[predecessor]) {
            nextpush (predecessor);
          }
        }
      }
    }
  }

  void QDSolver::run () {
    ingame.set ();
    game.vec_init ();

    for (pos = 0; pos < (uint) n_nodes; ++pos) {
      msr[pos] = 0;
      newsucc[pos] = -1;
      count[pos] = 0;
      weight[pos] = game.priority (pos);

      if (game.priority (pos) > 0) {
        TAtr.push (pos);
        BAtr[pos] = true;

        if (game.owner (pos) == 0) {
          newsucc[pos] = game.outvec (pos)[0];
        }
      } else {
        if (game.owner (pos) == 1) {
          count[pos] = game.outvec (pos).size ();
        }
      }
    }

    while (TAtr.nonempty () || TProm.nonempty ()) {
      atr ();
      promo ();
    }

    for (pos = 0; pos < (uint) n_nodes; ++pos) {
      if (ingame[pos]) {
        Solver::solve (pos, 1, strategy[pos]);
      } else {
        Solver::solve (pos, 0, strategy[pos]);
      }
    }

    game.vec_finish ();
  }
}
