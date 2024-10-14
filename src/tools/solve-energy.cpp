#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

using namespace std::string_literals;

#include "energy_game.hpp"

#include "verbose.hh"

#ifndef COMP_VARIANT
# define COMP_VARIANT potential_fvi
#endif

int               utils::verbose = 0;
utils::voutstream utils::vout;


class Solver {
  public:
    void solve (vertex_t v, bool winner, vertex_t strat) {
    }
};


#include "solvers/fvi.hpp"

void usage (const char* prog, const std::string& err) {
  if (not err.empty ())
    std::cerr << err << std::endl;
  std::cerr << "usage: " << prog << " FILENAME\n";
  exit (1);
}

int main (int argc, char** argv) {
  using solver_t = pg::FVISolver<potential::COMP_VARIANT>;
  // using solver_t = pg::FVISolver<potential::potential_fvi_qd>;
  // using solver_t = pg::FVISolver<potential::potential_fvi_phase1_pq>;
  // using solver_t = pg::FVISolver<potential::potential_fvi_nfvi>;

  using energy_game_t = solver_t::energy_game_t;
  using weight_t = solver_t::weight_t;
  using number_t = weight_t::number_t;

  energy_game_t nrg_game;

  // format:
  // energy #VERT
  // 1 [0/1] 3 -499129921,7 3920390932

  if (argc < 2)
    usage (argv[0], "not enough arguments");

  std::ifstream file (argv[1]);
  std::string line;
  size_t nverts;

  {
    if (not std::getline (file, line))
      usage (argv[0], "cannot get first line");
    std::istringstream iss (line);
    std::string magic;
    iss >> magic;
    if (magic != "energy"s)
      usage (argv[0], "magic isn't present");
    iss >> nverts;
    nrg_game.add_state (nverts - 1);
  }

  number_t max_w = 0;
  while (std::getline (file, line)) {
    std::istringstream iss (line);
    std::string result;
    vertex_t vert;
    int owner;
    iss >> vert >> owner >> std::ws;
    nrg_game.add_state (vert);
    if (owner == 0)
      nrg_game.make_max (vert);
    else
      nrg_game.make_min (vert);

    std::string t;
    while (getline (iss, t, ',')) {
      std::istringstream trans (t);
      vertex_t dest;
      number_t w;
      trans >> dest >> w;
      if (w > max_w)
        max_w = w;
      nrg_game.add_state (dest);
      nrg_game.add_transition (vert, w, dest);
    }
  }

  nrg_game.set_infty (weight_t (max_w * nverts));

  solver_t solver (nrg_game);
  solver.run ();
}
