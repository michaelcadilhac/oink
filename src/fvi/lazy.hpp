#pragma once

template <typename T>
class lazy_maker {
    inline static auto stash = std::vector<T> (1 << 15);
    inline static std::stack<size_t> free_Ts {};
    inline static size_t last_used = 0;

  public:
    inline static size_t used = 0;

    inline static T& get (size_t t) { return stash[t]; }

    static size_t make () {
      size_t ret;
      ++used;

      if (last_used < stash.size ())
        ret = last_used++;
      else
        if (not free_Ts.empty ()) {
          auto s = free_Ts.top ();
          free_Ts.pop ();
          ret = s;
        }
        else {
          stash.resize (2 * stash.size ());
          ret = last_used++;
        }
      return ret;
    }

    static void free (size_t t) {
      if (t == last_used - 1)
        --last_used;
      else
        free_Ts.push (t);
      --used;
    }
};


template <typename Number>
class lazy_number {
  public:
    using number_t = Number;

  private:
    size_t id;
    bool owns = true;

  private:
    lazy_number () : id (lazy_maker<number_t>::make ()), owns (true) {}
    lazy_number (const lazy_number& other) : lazy_number () { get () = *other; }
    lazy_number (size_t otherid, bool owns) : id (otherid), owns (owns) { }

  public:
    ~lazy_number () { if (owns) lazy_maker<number_t>::free (id); }

    lazy_number (lazy_number&& other) : id (other.id), owns (other.owns) {
      other.owns = false;
    }

    static lazy_number copy (const lazy_number& other) {
      return lazy_number (other); // complete copy
    }

    static lazy_number proxy (lazy_number& other) {
      return lazy_number (other.id, false); // shallow copy
    }

    static lazy_number steal (lazy_number& other) {
      bool was_owned = other.owns; // take ownership if other had it
      other.owns = false;          // ensure that other won't destroy id
      return lazy_number (other.id, was_owned);
    }

    lazy_number& copy_or_steal (lazy_number& other) {
      if (other.owns) { // steal
        if (owns)
          lazy_maker<number_t>::free (id);
        id = other.id;
        owns = true;
        other.owns = false;
      }
      else              // copy
        get () = *other;
      return *this;
    }

    lazy_number (const int64_t& src) : lazy_number () { get () = src; }
    lazy_number (const number_t& src) : lazy_number () { get () = src; }
    lazy_number (number_t&& src) : lazy_number () { get () = std::move (src); }

    lazy_number& operator= (lazy_number&& other) {
      if (owns)
        lazy_maker<number_t>::free (id);
      id = other.id;
      owns = other.owns;
      other.owns = false;
      return *this;
    }

    lazy_number& operator= (const lazy_number& other) {
      get () = *other;
      return *this;
    }
    lazy_number& operator= (int64_t other) {
      get () = other;
      return *this;
    }
    lazy_number& operator= (const number_t& other) {
      get () = other;
      return *this;
    }

    bool operator== (const lazy_number& other) const { return get () == *other; }
    bool operator<  (const lazy_number& other) const { return get () < *other; }
    bool operator<= (const lazy_number& other) const { return get () <= *other; }
    bool operator>  (const lazy_number& other) const { return get () > *other; }
    bool operator>= (const lazy_number& other) const { return get () >= *other; }

    bool operator== (int64_t other) const { return get () == other; }
    bool operator<  (int64_t other) const { return get () < other; }
    bool operator<= (int64_t other) const { return get () <= other; }
    bool operator>  (int64_t other) const { return get () > other; }
    bool operator>= (int64_t other) const { return get () >= other; }

    bool operator== (const number_t& other) const { return get () == other; }
    bool operator<  (const number_t& other) const { return get () < other; }
    bool operator<= (const number_t& other) const { return get () <= other; }
    bool operator>  (const number_t& other) const { return get () > other; }
    bool operator>= (const number_t& other) const { return get () >= other; }

    lazy_number& operator+= (const lazy_number& other)   { get () += *other; return *this; }
    lazy_number& operator+= (int64_t other)         { get () += other; return *this; }
    lazy_number& operator+= (const number_t& other) { get () += other; return *this; }

    lazy_number& operator-= (const lazy_number& other)   { get () -= *other; return *this; }
    lazy_number& operator-= (int64_t other)         { get () -= other; return *this; }
    lazy_number& operator-= (const number_t& other) { get () -= other; return *this; }

    number_t& operator* () { return get (); }
    const number_t& operator* () const { return get (); }

    number_t& get () { return lazy_maker<number_t>::get (id); }
    const number_t& get () const { return lazy_maker<number_t>::get (id); }
};


template <typename T>
std::ostream& operator<< (std::ostream& os, const lazy_number<T>& wt) {
  os << *wt;
  return os;
}

template <typename Number>
class active_number {
  public:
    using number_t = Number;

  private:
    number_t* num;
    bool owns = true;

  private:
    active_number () : num (new number_t ()), owns (true) {}
    active_number (const active_number& other) : active_number () { get () = *other; }
    active_number (number_t* othernum, bool owns) : num (othernum), owns (owns) { }

  public:
    ~active_number () { if (owns) delete num; }

    active_number (active_number&& other) : num (other.num), owns (other.owns) {
      other.owns = false;
    }

    static active_number copy (const active_number& other) {
      return active_number (other); // complete copy
    }

    static active_number proxy (active_number& other) {
      return active_number (other.num, false); // shallow copy
    }

    static active_number steal (active_number& other) {
      bool was_owned = other.owns; // take ownership if other had it
      other.owns = false;          // ensure that other won't destroy num
      return active_number (other.num, was_owned);
    }

    active_number& copy_or_steal (active_number& other) {
      if (other.owns) { // steal
        if (owns)
          delete num;
        num = other.num;
        owns = true;
        other.owns = false;
      }
      else              // copy
        get () = *other;
      return *this;
    }

    active_number (const int64_t& src) : active_number () { get () = src; }
    active_number (const number_t& src) : active_number () { get () = src; }
    active_number (number_t&& src) : active_number () { get () = std::move (src); }

    active_number& operator= (active_number&& other) {
      if (owns)
        delete num;
      num = other.num;
      owns = other.owns;
      other.owns = false;
      return *this;
    }

    active_number& operator= (const active_number& other) {
      get () = *other;
      return *this;
    }
    active_number& operator= (int64_t other) {
      get () = other;
      return *this;
    }
    active_number& operator= (const number_t& other) {
      get () = other;
      return *this;
    }

    bool operator== (const active_number& other) const { return get () == *other; }
    bool operator<  (const active_number& other) const { return get () < *other; }
    bool operator<= (const active_number& other) const { return get () <= *other; }
    bool operator>  (const active_number& other) const { return get () > *other; }
    bool operator>= (const active_number& other) const { return get () >= *other; }

    bool operator== (int64_t other) const { return get () == other; }
    bool operator<  (int64_t other) const { return get () < other; }
    bool operator<= (int64_t other) const { return get () <= other; }
    bool operator>  (int64_t other) const { return get () > other; }
    bool operator>= (int64_t other) const { return get () >= other; }

    bool operator== (const number_t& other) const { return get () == other; }
    bool operator<  (const number_t& other) const { return get () < other; }
    bool operator<= (const number_t& other) const { return get () <= other; }
    bool operator>  (const number_t& other) const { return get () > other; }
    bool operator>= (const number_t& other) const { return get () >= other; }

    active_number& operator+= (const active_number& other)   { get () += *other; return *this; }
    active_number& operator+= (int64_t other)         { get () += other; return *this; }
    active_number& operator+= (const number_t& other) { get () += other; return *this; }

    active_number& operator-= (const active_number& other)   { get () -= *other; return *this; }
    active_number& operator-= (int64_t other)         { get () -= other; return *this; }
    active_number& operator-= (const number_t& other) { get () -= other; return *this; }

    number_t& operator* () { return get (); }
    const number_t& operator* () const { return get (); }

    number_t& get () { return *num; }
    const number_t& get () const { return *num; }
};



template <typename T>
std::ostream& operator<< (std::ostream& os, const active_number<T>& wt) {
  os << *wt;
  return os;
}
