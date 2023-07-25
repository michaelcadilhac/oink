/*
 * Copyright 2022 Tom van Dijk
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

#include <iostream>

#include "game.hpp"

using namespace pg;

int
main(int argc, char** argv)
{
    if (argc != 2) {
        std::cout << "Matthew Maat's counterexample to Schewe et al's symmetric strategy improvement." << std::endl;
        std::cout << "Syntax: " << argv[0] << " N" << std::endl;
        return -1;
    }

    int n = std::stoi(argv[1]);
    int N = 16*n+16;

    Game game(2+n*10);
    game.vec_init();

    /* create n pieces */
    for (int i=0; i<n; i++) {
        int a = 10*i+0;
        int d = 10*i+1;
        int c = 10*i+2;
        int e = 10*i+3;
        int m = 10*i+4;
        int f = 10*i+5;
        int g = 10*i+6;
        int k = 10*i+7;
        int h = 10*i+8;
        int l = 10*i+9;
        /* a */ game.init_vertex(a, N+2*i-1, 0);    
        /* d */ game.init_vertex(d, N+2*i,   1);
        /* c */ game.init_vertex(c, 14*i+1,  0);
        /* e */ game.init_vertex(e, 14*i+4,  1);
        /* m */ game.init_vertex(m, 14*i+3,  0);
        /* f */ game.init_vertex(f, 14*i+6,  1);
        /* g */ game.init_vertex(g, 14*i+8,  1);
        /* k */ game.init_vertex(k, 14*i+11, 0);
        /* h */ game.init_vertex(h, 14*i+10, 1);
        /* l */ game.init_vertex(l, 14*i+13, 0);
    }

    game.init_vertex(10*n,   1,       0);
    game.init_vertex(10*n+1, N+2*n+2, 1);

    /* connect the pieces */
    for (int i=0; i<n; i++) {
        int a = 10*i+0;
        int d = 10*i+1;
        int c = 10*i+2;
        int e = 10*i+3;
        int m = 10*i+4;
        int f = 10*i+5;
        int g = 10*i+6;
        int k = 10*i+7;
        int h = 10*i+8;
        int l = 10*i+9;

        game.vec_add_edge(a, c);
        game.vec_add_edge(d, h);
        game.vec_add_edge(c, e);
        game.vec_add_edge(c, m);
        game.vec_add_edge(c, 0);
        game.vec_add_edge(m, f);
        game.vec_add_edge(m, c);
        game.vec_add_edge(m, 0);
        game.vec_add_edge(g, k);
        game.vec_add_edge(g, h);
        game.vec_add_edge(g, 1);
        game.vec_add_edge(h, l);
        game.vec_add_edge(h, g);
        game.vec_add_edge(h, 1);
        game.vec_add_edge(e, m);
        game.vec_add_edge(e, a+10);
        game.vec_add_edge(f, c);
        game.vec_add_edge(f, d+10);
        game.vec_add_edge(k, h);
        game.vec_add_edge(k, a+10);
        game.vec_add_edge(l, g);
        game.vec_add_edge(l, d+10);
    }

    game.vec_add_edge(10*n,   10*n);
    game.vec_add_edge(10*n+1, 10*n);

    game.vec_finish();
    game.write_pgsolver(std::cout);
}
