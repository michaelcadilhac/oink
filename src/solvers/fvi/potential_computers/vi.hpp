#pragma once

namespace potential_computers {
  template <typename W>
  class potential_firstmax : public potential_computer<W> {
    public:
      potential_firstmax (const energy_game<W>& game, logger_t& logger, int trace) :
        potential_computer<W> (game, logger, trace) {}

      void compute () {
        auto& potential = this->potential;
        auto& game = this->game;

        for (auto& p : potential)
          p = game.get_infty ();

        for (auto&& v : game.undecided_vertices ()) {
          if (game.is_max (v) ) {
            potential[v] = game.outs (v)[0].first;
            for (auto& tr : game.outs (v) | std::views::drop (1)) {
              if (potential[v] < tr.first)
                potential[v] = tr.first;
            }
          }
          else {
            potential[v] = game.outs (v)[0].first;
            for (auto& tr : game.outs (v) | std::views::drop (1)) {
              if (potential[v] > tr.first)
                potential[v] = tr.first;
            }
          }
          if (potential[v] < 0)
            potential[v] = zero_weight<W> (game.get_infty ());
        }
      }
  };
}
