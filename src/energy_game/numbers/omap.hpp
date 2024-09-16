#pragma once

#include "omap_cmp.hpp"

template <typename T, typename U>
class omap {
    using map_t = std::map<T, U, std::greater<T>>; // map priority -> value, ordered by largest priority.
    map_t map;
    omap (map_t&& map) : map (std::move (map)) {};
    friend class omap_cmp;
    using enum omap_cmp::cmp_res;

  public:
    omap () {}
    omap (omap&& other) : map (std::move (other.map)) {};
    omap (const omap& other) : map (other.map) {};

    omap& operator+= (const omap& other) {
      for (auto&& p : other.map)
        map[p.first] += p.second;
      return *this;
    }

    omap& operator-= (const omap& other) {
      for (auto&& p : other.map) {
        map[p.first] -= p.second;
      }
      return *this;
    }

    // (a <=> b) < 0 iff a < b
    U operator<=> (const omap& other) const {
      auto itself = map.begin ();
      auto itother = other.map.cbegin ();

      auto ret = [&]<bool Lhs, bool P1, bool P2> (omap_cmp::cmp_res ret) {
        switch (ret) {
          case SUM_GREATER: return -1;
          case LHS_GREATER: return 1;
          case EQUAL:       return 0;
          default:          assert (false); return 0;
        }
      };

      return omap_cmp::deep_cmp (*this, other, itself, itother, ret);
    }

    bool operator== (const omap& other) const { return (*this <=> other) == 0; }

    U operator<=> (const U& other) const {
      assert (other == 0);
      for (auto&& x : map) {
        if (x.second == 0)
          continue;
        return x.second;
      }
      return 0;
    }
    bool operator== (const U& other) const { return (*this <=> other) == 0; }

    omap& operator= (omap&& other) {
      map = std::move (other.map);
      return *this;
    }

    omap& operator= (const omap& other) {
      map = other.map;
      return *this;
    }

    omap operator- () const {
      map_t othermap (map);
      for (auto&& x : othermap)
        x.second = -x.second;
      return omap (std::move (othermap));
    }

    bool set_if_plus (const omap& p1, const omap& p2, omap_cmp::cmp_res condition) {
      auto itlhs = map.cbegin();
      auto itp1 = p1.map.cbegin(),
        itp2 = p2.map.cbegin ();

      auto copy_impl = [&]<bool Lhs, bool P1, bool P2> () {
        if (Lhs)
          map.erase (itlhs, map.cend ());
        if (P1 and not P2) {
          map.insert (itp1, p1.map.cend ());
          itp1 = p1.map.cend ();
          return;
        }
        if (P2 and not P1) {
          map.insert (itp2, p2.map.cend ());
          itp2 = p2.map.cend ();
          return;
        }
        // P1 and P2: add
        while (itp1 != p1.map.end () and itp2 != p2.map.end ()) {
          if (itp1->first > itp2->first) {
            map[itp1->first] = itp1->second;
            ++itp1;
          }
          else if (itp2->first > itp1->first) {
            map[itp2->first] = itp2->second;
            ++itp2;
          }
          else {// (itp1->first == itp2->first)
            map[itp1->first] = itp1->second + itp2->second;
            ++itp1, ++itp2;
          }
        }
      };

      auto copy = [&]<bool Lhs, bool P1, bool P2> (omap_cmp::cmp_res ret) {
        if (ret == condition) {
          copy_impl.template operator()<Lhs, P1, P2> ();
          if (P1 and itp1 != p1.map.end ())
            copy_impl.template operator()<false, true, false> ();
          else if (P2 and itp2 != p2.map.end ())
            copy_impl.template operator()<false, false, true> ();
          assert ((not P1 or itp1 == p1.map.end ()) and
                  (not P2 or itp2 == p2.map.end ()));
          return true;
        }
        return false;
      };

      return omap_cmp::deep_cmp (*this, p1, p2, itlhs, itp1, itp2, copy);
    }

    bool set_if_plus_larger (const omap& p1, const omap& p2) {
      return set_if_plus (p1, p2, SUM_GREATER);
    }

    bool set_if_plus_smaller (const omap& p1, const omap& p2) {
      return set_if_plus (p1, p2, LHS_GREATER);
    }

    static omap priority_to_number (const priority_t& prio,
                                    const pg::Game&,
                                    bool swap) {
#ifdef GAMES_ARE_NRG
      static_assert (false, "omap is not a valid type for native energy games.");
#endif

      typename omap::map_t map;
      map[prio] = (swap ^ (prio % 2)) ? -1 : 1;
      return omap (std::move (map));
    }

    static omap infinity_number (const pg::Game& pgame) {
      auto max_prio = pgame.priority (pgame.nodecount () - 1);
      typename omap::map_t map;
      map[max_prio] = pgame.edgecount () + 1;
      return omap (std::move (map));
    }

    static omap zero_number (const omap&) {
      return omap ();
    }

    std::ostream& print (std::ostream& os) const {
      os << "[";
      for (auto&& x : map)
        os << " " << x.first << "->" << x.second;
      os << " ]";
      return os;
    }
};
