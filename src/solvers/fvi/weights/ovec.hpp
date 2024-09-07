#pragma once

template <typename T>
class ovec {
    using vec_t = std::vector<T>;
    std::vector<T> vec;
    ovec (vec_t&& vec) : vec (std::move (vec)) {};
    ovec (size_t sz) : vec (sz) {}

  public:
    ovec () {}
    ovec (ovec&& other) : vec (std::move (other.vec)) {};
    ovec (const ovec& other) : vec (other.vec) {};


    ovec& operator+= (const ovec& other) {
      for (auto itA = vec.begin(), itB = other.vec.begin();
           itA != vec.end(); (++itA, ++itB)) {
        *itA += *itB;
      }
      return *this;
    }

    ovec& operator-= (const ovec& other) {
      for (auto itA = vec.begin(), itB = other.vec.begin();
           itA != vec.end(); (++itA, ++itB)) {
        *itA -= *itB;
      }
      return *this;
    }

    T operator<=> (const ovec& other) const {
      for (auto itA = vec.begin(), itB = other.vec.begin();
           itA != vec.end(); (++itA, ++itB)) {
        T ret = *itA - *itB;
        if (ret != 0)
          return ret;
      }
      return 0;
    }
    bool operator== (const ovec& other) const { return (*this <=> other) == 0; }
    T operator<=> (const T& other) const {
      assert (other == 0);
      for (auto&& x : vec) {
        if (x == 0)
          continue;
        return x;
      }
      return 0;
    }
    bool operator== (const T& other) const { return (*this <=> other) == 0; }

    ovec& operator= (ovec&& other) {
      vec = std::move (other.vec);
      return *this;
    }

    ovec& operator= (const ovec& other) {
      vec = other.vec;
      return *this;
    }

    ovec operator- () const {
      vec_t othervec (vec.size ());
      auto itA = vec.begin();
      auto itB = othervec.begin();
      for (; itA != vec.end(); (++itA, ++itB)) {
        *itB = -*itA;
      }
      return ovec (std::move (othervec));
    }

  private:
    bool set_if_plus (const ovec& p1, const ovec& p2, bool larger) {
      bool decided_it_is_sum = false;

      assert (vec.size () == p1.vec.size () and p2.vec.size () == vec.size ());

      auto itlhs = vec.begin();
      auto itp1 = p1.vec.begin();
      auto itp2 = p2.vec.begin ();

      for (; itlhs != vec.end(); ++itlhs, ++itp1, ++itp2) {
        auto tmp = *itp1 + *itp2;
        if (not decided_it_is_sum) {
          auto eq = tmp <=> *itlhs;
          if (larger ? (eq < 0) : (eq > 0))
            return false;
          decided_it_is_sum = larger ? (eq > 0) : (eq < 0);
        }
        if (decided_it_is_sum)
          *itlhs = tmp;
      }
      return true;
    }

  public:
    bool set_if_plus_larger (const ovec& p1, const ovec& p2) { return set_if_plus (p1, p2, true); }
    bool set_if_plus_smaller (const ovec& p1, const ovec& p2) { return set_if_plus (p1, p2, false); }

    static ovec priority_to_weight (const priority_t& prio,
                                    const pg::Game& pgame,
                                    bool swap) {
      auto max_prio = pgame.priority (pgame.nodecount () - 1);
      vec_t vec (max_prio + 1);
      vec[max_prio - prio] = (swap ^ (prio % 2)) ? -1 : 1;
      return std::move (vec);
    }

    static ovec infinity_weight (const pg::Game& pgame) {
      auto max_prio = pgame.priority (pgame.nodecount () - 1);
      vec_t vec (max_prio + 1);
      vec[0] = pgame.edgecount () + 1;
      return std::move (vec);
    }

    static ovec zero_weight (const ovec& other) {
      return other.vec.size (); // initialized to 0
    }

    std::ostream& print (std::ostream& os) const {
      os << "[";
      priority_t prio = 0;
      for (auto&& x : std::views::reverse (vec))
        os << " " << prio++ << "->" << x;
      os << " ]";
      return os;
    }
};
