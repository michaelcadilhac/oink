#pragma once

template <typename T, typename U>
class omap;



class omap_cmp {
    /** These functions take three Boolean parameters, indicating which side
     * still has elements. */
  public:
    enum class cmp_res {
      SUM_GREATER,
      LHS_GREATER,
      EQUAL,
      CONTINUE,
      UNDECIDED
    };
    using enum omap_cmp::cmp_res;

    template <bool Lhs, bool P1, bool P2>
    static inline auto cmp (auto& itlhs, auto& itp1, auto& itp2) {
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

    template <instantiation_of<omap> OW, bool Lhs, bool P1, bool P2>
    static auto loop1 (const OW& lhs, const OW& p1, const OW& p2,
                       typename OW::map_t::const_iterator& itlhs,
                       typename OW::map_t::const_iterator& itp1,
                       typename OW::map_t::const_iterator& itp2) {
      while ((not Lhs or itlhs != lhs.map.end ()) and
             (not P1 or itp1 != p1.map.end ()) and
             (not P2 or itp2 != p2.map.end ())) {
        switch (cmp<Lhs, P1, P2> (itlhs, itp1, itp2)) {
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

#define RET_IF_DECIDED(Lhs, P1, P2) do {                                \
      auto ret = loop1<OW, Lhs, P1, P2> (lhs, p1, p2, itlhs, itp1, itp2); \
      if (ret != UNDECIDED)                                             \
        return action.template operator()<Lhs, P1, P2> (ret);           \
    } while (0)

  public:
    template <instantiation_of<omap> OW>
    static auto deep_cmp (const OW& lhs, const OW& p1,
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
    static auto deep_cmp (const OW& lhs, const OW& p1, const OW& p2,
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
        return deep_cmp (lhs, p2, itlhs, itp2, action);
      else {
        assert (itp2 == p2.map.end ());
        return deep_cmp (lhs, p1, itlhs, itp1, action);
      }
    }
#undef RET_IF_DECIDED
};
