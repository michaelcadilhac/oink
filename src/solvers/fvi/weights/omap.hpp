#pragma once



enum omap_res {
  SUM_GREATER,
  LHS_GREATER,
  EQUAL,
  CONTINUE,
  UNDECIDED
};

template <typename T, typename U>
class omap;

class omap_helper {
  public:
    template <bool Lhs, bool P1, bool P2>
    static inline auto omap_cmp  (auto& itlhs, auto& itp1, auto& itp2) {
      assert (Lhs or P1 or P2);

      auto rhs_prio = std::max (P1 ? itp1->first : 0, P2 ? itp2->first : 0);
      auto rhs_value =
        ((P1 and itp1->first == rhs_prio) ? itp1->second : 0) +
        ((P2 and itp2->first == rhs_prio) ? itp2->second : 0);
      auto lhs_prio = Lhs ? itlhs->first : rhs_prio;
      auto lhs_value = Lhs ? itlhs->second : 0;

      if (lhs_prio > rhs_prio) {
        if (lhs_value < 0)
          return SUM_GREATER;
        if (lhs_value > 0)
          return LHS_GREATER;
        // lhs_value is zero, continue.
        assert (Lhs);
        ++itlhs;
        return CONTINUE;
      }
      if (lhs_prio < rhs_prio) {
        if (rhs_value < 0)
          return LHS_GREATER;
        if (rhs_value > 0)
          return SUM_GREATER;
        if (P1 and itp1->first == rhs_prio)
          ++itp1;
        if (P2 and itp2->first == rhs_prio)
          ++itp2;
        return CONTINUE;
      }
      // lhs_prio == rhs_prio
      if (lhs_value > rhs_value)
        return LHS_GREATER;
      if (lhs_value < rhs_value)
        return SUM_GREATER;
      if (Lhs)
        ++itlhs;
      if (P1 and itp1->first == rhs_prio)
        ++itp1;
      if (P2 and itp2->first == rhs_prio)
        ++itp2;
      return CONTINUE;
    }

  private:
    template <bool Lhs, bool P1, bool P2, instantiation_of<omap> T>
    static auto omap_loop1  (const T& lhs, const T& p1, const T& p2,
                             typename T::map_t::const_iterator& itlhs,
                             typename T::map_t::const_iterator& itp1,
                             typename T::map_t::const_iterator& itp2) {
      while ((not Lhs or itlhs != lhs.map.end ()) and
             (not P1 or itp1 != p1.map.end ()) and
             (not P2 or itp2 != p2.map.end ())) {
        switch (omap_cmp<Lhs, P1, P2> (itlhs, itp1, itp2)) {
          case SUM_GREATER:
            return SUM_GREATER;
          case LHS_GREATER:
            return LHS_GREATER;
          case CONTINUE:
            continue;
          default:
            assert (false);
        }
      }
      if ((not Lhs or itlhs == lhs.map.end ()) and
          (not P1 or itp1 == p1.map.end ()) and
          (not P2 or itp2 == p2.map.end ()))
        return EQUAL; // equal

      return UNDECIDED;
    }

  public:

#define RET_IF_DECIDED(Lhs, P1, P2) do {                                \
      auto ret = omap_helper::omap_loop1<Lhs, P1, P2> (lhs, p1, p2, itlhs, itp1, itp2); \
      if (ret != UNDECIDED)                                             \
        return action.template operator()<Lhs, P1, P2> (ret);           \
    } while (0)

    template <instantiation_of<omap> OW>
    static auto omap_deep_cmp (const OW& lhs, const OW& p1,
                               typename OW::map_t::const_iterator& itlhs,
                               typename OW::map_t::const_iterator& itp1,
                               auto& action) {
      auto& p2 = p1;
      auto& itp2 = itp1;

      RET_IF_DECIDED (true, true, false);
      if (itp1 == p1.map.end ())
        RET_IF_DECIDED (true, false, false);
      else
        RET_IF_DECIDED (false, true, false);
      assert (false);
      return action.template operator()<false, false, false> (UNDECIDED);
    }

    template <instantiation_of<omap> OW>
    static auto omap_deep_cmp (const OW& lhs, const OW& p1, const OW& p2,
                               typename OW::map_t::const_iterator& itlhs,
                               typename OW::map_t::const_iterator& itp1,
                               typename OW::map_t::const_iterator& itp2,
                               auto& action) {
      RET_IF_DECIDED (true, true, true);
      assert (itlhs != lhs.map.end () or
              itp1 != p1.map.end () or
              itp2 != p2.map.end ());
      if (itlhs == lhs.map.end ()) {
        RET_IF_DECIDED (false, true, true);
        if (itp1 == p1.map.end ())
          RET_IF_DECIDED (false, false, true);
        else
          RET_IF_DECIDED (false, true, false);
        assert (false);
        return false;
      }

      if (itp1 == p1.map.end ())
        return omap_deep_cmp (lhs, p2, itlhs, itp2, action);
      else {
        assert (itp2 == p2.map.end ());
        return omap_deep_cmp (lhs, p1, itlhs, itp1, action);
      }
    }
#undef RET_IF_DECIDED
};

template <typename T, typename U>
std::ostream& operator<< (std::ostream& os, const omap<T, U>& wt) {
  os << "[";
  for (auto&& x : wt.map)
    os << " " << x.first << "->" << x.second;
  os << " ]";
  return os;
}


template <typename T, typename U>
class omap {
    using map_t = std::map<T, U, std::greater<T>>; // map priority -> value, ordered by largest priority.
    map_t map;
    omap (map_t&& map) : map (std::move (map)) {};

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
      for (auto&& p : other.map)
        map[p.first] -= p.second;
      return *this;
    }

    // (a <=> b) < 0 iff a < b
    U operator<=> (const omap& other) const {
      auto itself = map.begin ();
      auto itother = other.map.cbegin ();

      auto ret = [&]<bool Lhs, bool P1, bool P2> (int ret) {
        switch (ret) {
          case SUM_GREATER: return -1;
          case LHS_GREATER: return 1;
          case EQUAL:       return 0;
          default:
          assert (false);
          return 0;
        }
      };

      return omap_helper::omap_deep_cmp (*this, other, itself, itother, ret);
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

    friend bool set_if_plus<> (omap&, const omap&, const omap&, int);

    friend omap priority_to_weight<> (const priority_t&,
                                      const pg::Game&,
                                      bool swap);

    friend omap infinity_weight<> (const pg::Game&);

    friend omap zero_weight<> (const omap&);

    friend std::ostream& operator<< <> (std::ostream& os, const omap<T, U>& wt);

    friend class omap_helper;
};

template <instantiation_of<omap> OW>
bool set_if_plus (OW& lhs, const OW& p1, const OW& p2, int condition) {
  auto itlhs = lhs.map.cbegin();
  auto itp1 = p1.map.cbegin(),
    itp2 = p2.map.cbegin ();

  auto copy_impl = [&]<bool Lhs, bool P1, bool P2> () {
    if (Lhs)
      lhs.map.erase (itlhs, lhs.map.cend ());
    if (P1 and not P2) {
      lhs.map.insert (itp1, p1.map.cend ());
      itp1 = p1.map.cend ();
      return;
    }
    if (P2 and not P1) {
      lhs.map.insert (itp2, p2.map.cend ());
      itp2 = p2.map.cend ();
      return;
    }
    // P1 and P2: add
    while (itp1 != p1.map.end () and itp2 != p2.map.end ()) {
      if (itp1->first > itp2->first) {
        lhs.map[itp1->first] = itp1->second;
        ++itp1;
      }
      else if (itp2->first > itp1->first) {
        lhs.map[itp2->first] = itp2->second;
        ++itp2;
      }
      else {// (itp1->first == itp2->first)
        lhs.map[itp1->first] = itp1->second + itp2->second;
        ++itp1, ++itp2;
      }
    }
  };

  auto copy = [&]<bool Lhs, bool P1, bool P2> (int ret) {
    if (ret == condition) {
      copy_impl.template operator()<Lhs, P1, P2> ();
      if (P1 and itp1 != p1.map.end ())
        copy_impl.template operator()<false, true, false> ();
      else if (P2 and itp2 != p2.map.end ())
        copy_impl.template operator()<false, false, true> ();
        assert (itp1 == p1.map.end () and itp2 == p2.map.end ());
        return true;
    }
    return false;
  };

  return omap_helper::omap_deep_cmp (lhs, p1, p2, itlhs, itp1, itp2, copy);
}

template <instantiation_of<omap> OW>
bool set_if_plus_larger (OW& lhs, const OW& p1, const OW& p2) {
  return set_if_plus (lhs, p1, p2, SUM_GREATER);
}

template <instantiation_of<omap> OW>
bool set_if_plus_smaller (OW& lhs, const OW& p1, const OW& p2) {
  return set_if_plus (lhs, p1, p2, LHS_GREATER);
}

template <instantiation_of<omap> OW>
OW priority_to_weight (const priority_t& prio,
                       const pg::Game& pgame,
                       bool swap) {
  typename OW::map_t map;
  map[prio] = (swap ^ (prio % 2)) ? -1 : 1;
  return OW (std::move (map));
}

template <instantiation_of<omap> OW>
OW infinity_weight (const pg::Game& pgame) {
  auto max_prio = pgame.priority (pgame.nodecount () - 1);
  typename OW::map_t map;
  map[max_prio] = pgame.edgecount () + 1;
  return OW (std::move (map));
}

template <instantiation_of<omap> OW>
OW zero_weight (const OW&) {
  return OW ();
}
