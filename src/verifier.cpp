/*
 * Copyright 2017-2018 Tom van Dijk, Johannes Kepler University Linz
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

#include <algorithm>
#include <cassert>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <queue>
#include <deque>
#include <stack>

#include "verifier.hpp"

using namespace std;

namespace pg {

void
Verifier::verify(bool fullgame, bool even, bool odd)
{
    // ensure the vertices are ordered properly
    game->ensure_sorted();
    // ensure that the arrays are built
    game->build_in_array(false);

    const int n_vertices = game->vertexcount();

    /**
     * The first loop removes all edges from "won" vertices that are not the strategy.
     * This turns each dominion into a single player game.
     * Also some trivial checks are performed.
     */
    for (int i=0; i<n_vertices; i++) {
        // (for full solutions) check whether every vertex is won
        if (!game->solved[i]) {
            if (fullgame) throw "not every vertex is won";
            else continue;
        }

        const bool winner = game->winner[i];

        if (winner == 0 and !even) continue; // whatever
        if (winner == 1 and !odd) continue; // whatever

        if (winner == game->owner(i)) {
            // if winner, check whether the strategy stays in the dominion
            int str = game->strategy[i];
            if (str == -1) {
                throw "winning vertex has no strategy";
            } else if (!game->has_edge(i, str)) {
                throw "strategy is not a valid move";
            } else if (!game->solved[str] or game->winner[str] != winner) {
                throw "strategy leaves dominion";
            }
            n_strategies++; // number of checked strategies
        } else {
            // if loser, check whether the loser can escape
            for (auto curedge = game->outs(i); *curedge != -1; curedge++) {
                int to = *curedge;
                if (!game->solved[to] or game->winner[to] != winner) {
                    logger << "escape edge from " << game->label_vertex(i) << " to " << game->label_vertex(to) << std::endl;
                    throw "loser can escape";
                }
            }
            // and of course check that no strategy is set
            if (game->strategy[i] != -1) throw "losing vertex has strategy";
        }
    }

    // Allocate datastructures for Tarjan search
    int *done = new int[n_vertices];
    int64_t *low = new int64_t[n_vertices];
    for (int i=0; i<n_vertices; i++) done[i] = -1;
    for (int i=0; i<n_vertices; i++) low[i] = 0;

    std::vector<int> res;
    std::stack<int> st;

    int64_t pre = 0;

    for (int i=n_vertices-1; i>=0; i--) {
        // only if a dominion
        if (!game->solved[i]) continue;

        int prio = game->priority(i);
        int winner = game->winner[i];

        // only compute SCC for a (probably) top vertex
        if (winner == 0 and !even) continue; // don't check even dominions
        if (winner == 1 and !odd) continue; // don't check odd dominions

        // only try to find an SCC where the loser wins
        if (winner == (prio&1)) continue;

        // only run the check if not yet done at priority <prio>
        if (done[i] == prio) continue;

        // set <bot> (in tarjan search) to current pre
        int64_t bot = pre;

        // start the tarjan search at vertex <i>
        st.push(i);

        while (!st.empty()) {
            int v = st.top();

            /**
             * When we see it for the first item, we assign the next number to it and add it to <res>.
             */
            if (low[v] <= bot) {
                low[v] = ++pre;
                res.push_back(v);
            }

            /**
             * Now we check all outgoing (allowed) edges.
             * If seen earlier, then update "min"
             * If new, then 'recurse'
             */
            int min = low[v];
            bool pushed = false;
            if (game->strategy[v] != -1) {
                int to = game->strategy[v];
                if (to > i) {
                    // skip if to higher priority
                } else if (done[to] == prio) {
                    // skip if already found scc (done[to] set to prio)
                } else if (low[to] <= bot) {
                    // not visited, add to <st> and break!
                    st.push(to);
                    pushed = true;
                } else {
                    // visited, update min
                    if (low[to] < min) min = low[to];
                }
            } else {
                for (auto curedge = game->outs(v); *curedge != -1; curedge++) {
                    int to = *curedge;
                    // skip if to higher priority
                    if (to > i) continue;
                    // skip if already found scc (done[to] set to prio)
                    if (done[to] == prio) continue;
                    // check if visited in this search
                    if (low[to] <= bot) {
                        // not visited, add to <st> and break!
                        st.push(to);
                        pushed = true;
                        break;
                    } else {
                        // visited, update min
                        if (low[to] < min) min = low[to];
                    }
                }
            }
            if (pushed) continue; // we pushed a new vertex to <st>...

            // there was no edge to a new vertex
            // check if we are the root of a SCC
            if (min < low[v]) {
                // not the root (outgoing edge to lower ranked vertex)
                low[v] = min;
                st.pop();
                continue;
            }

            /**
             * We're the root!
             * Now we need to figure out if we have cycles with p...
             * Also, mark every vertex in the SCC as "done @ search p"
             */
            int max_prio = -1;
            int scc_size = 0;
            for (auto it=res.rbegin(); it!=res.rend(); it++) {
                int n = *it;
                if (game->priority(n) > max_prio) max_prio = game->priority(n);
                scc_size++;
                done[n] = prio; // mark as done at prio
                if (n == v) break;
            }

            bool cycles = scc_size > 1 or game->strategy[v] == v or
                (game->strategy[v] == -1 and game->has_edge(v, v));

            if (cycles && (max_prio&1) == (prio&1)) {
                /**
                 * Found! Report.
                 */
                logger << "\033[1;31mscc where loser wins\033[m with priority \033[1;34m" << max_prio << "\033[m";
                for (auto it=res.rbegin(); it!=res.rend(); it++) {
                    int n = *it;
                    logger << " " << n;
                    if (n == v) break;
                }
                logger << std::endl;
                delete[] done;
                delete[] low;

                throw "loser can win";
            }

            /**
             * Not found! Continue.
             */
            for (;;) {
                int n = res.back();
                res.pop_back();
                if (n == v) break;
            }
            st.pop();
        }
    }

    delete[] done;
    delete[] low;
}

}
