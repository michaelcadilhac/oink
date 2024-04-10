#pragma once


template <typename T>
class ovec;


template <typename T>
std::ostream& operator<< (std::ostream& os, const ovec<T>& wt) {
  os << "[";
  priority_t prio = 0;
  for (auto&& x : std::views::reverse (wt.vec))
    os << " " << prio++ << "->" << x;
  os << " ]";
  return os;
}


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

    friend bool set_if_plus_larger<> (ovec&, const ovec&, const ovec&);

    friend ovec priority_to_weight<> (const priority_t&,
                                      const pg::Game&,
                                      bool swap);

    friend ovec infinity_weight<> (const pg::Game&);

    friend ovec zero_weight<> (const ovec&);

    friend std::ostream& operator<< <> (std::ostream& os, const ovec<T>& wt);
};


template <instantiation_of<ovec> OW>
bool set_if_plus_larger (OW& lhs, const OW& p1, const OW& p2) {
  static typename OW::vec_t vec;
  vec.resize (p1.vec.size ());

  bool gt = false;

  assert (lhs.vec.size () == p1.vec.size () and p2.vec.size () == vec.size ());

  auto itlhs = lhs.vec.begin();
  auto itp1 = p1.vec.begin();
  auto itp2 = p2.vec.begin ();
  auto itout = vec.begin ();

  for (; itlhs != lhs.vec.end(); ++itlhs, ++itp1, ++itp2, ++itout) {
    auto tmp = *itp1 + *itp2;
    if (not gt) {
      auto eq = tmp <=> *itlhs;
      if (eq < 0)
        return false;
      gt = (eq > 0);
    }
    if (gt)
      *itlhs = tmp;
  }
  return true;
}

template <instantiation_of<ovec> OW>
OW priority_to_weight (const priority_t& prio,
                            const pg::Game& pgame,
                            bool swap) {
  auto max_prio = pgame.priority (pgame.nodecount () - 1);
  typename OW::vec_t vec (max_prio + 1);
  vec[max_prio - prio] = (swap ^ (prio % 2)) ? -1 : 1;
  return OW (std::move (vec));
}

template <instantiation_of<ovec> OW>
OW infinity_weight (const pg::Game& pgame) {
  auto max_prio = pgame.priority (pgame.nodecount () - 1);
  typename OW::vec_t vec (max_prio + 1);
  vec[0] = pgame.edgecount () + 1;
  return OW (std::move (vec));
}

template <instantiation_of<ovec> OW>
OW zero_weight (const OW& other) {
  return OW (other.vec.size ()); // intialized to 0
}
