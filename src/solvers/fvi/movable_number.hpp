#pragma once

/**
 * @file movable_number.hpp
 * @brief A type of reference to numbers where the reference can be owned
 * or not owned.  Allows copying (owned), stealing (owned if the source was),
 * proxying (not owned), and steal-or-copying (owned, stolen if the other is
 * owned, copied otherwise).
 *
 * The abstract class @a movable_number implements generic operators and methods
 * and takes as type arguments the exact type of the derived class, and the type
 * of the underlying numbers (e.g., @a int32_t, @a mpz_int, ...).
 *
 * Concrete classes should look like this:
 *
 * @snippet this S1
 */
#if 0
// [S1]
class concrete_number final : public movable_number<concrete_number, underlying_number_type> {
  private:
    using mn_t = movable_number<concrete_number, underlying_number_type>;
    friend mn_t;

  private:
    concrete_number () { mn_t::owns = true; /* ... */}
    concrete_number (const concrete_number& other) { /* make a deep, owned copy of other */ }
    concrete_number (const concrete_number& other, bool owns) {
      mn_t::owns = owns;
      /* make a shallow reference to other */
    }

  public:
    using number_t = Number;
    using mn_t::operator=;

    void destroy () { /* free ressources of that number that is guaranteed to own */ }

    concrete_number (concrete_number&& other) { /* move constructor */ }
    concrete_number (const int64_t& src) { /* owning an int64_t */ }
    concrete_number (const number_t& src) { /* owning a number_t */ }
    concrete_number (number_t&& src) { /* owning a moved number_t */ }

    concrete_number& operator= (concrete_number&& other) {
      /* same as move constructor, but may destroy current */
    }
    concrete_number& operator= (const concrete_number& other) {
      /* set the current number to another value, no change in ownership, should not destroy anything */
    }

    number_t&       get ()       { /* return current number */ }
    const number_t& get () const { /* return current number */ }
};
// [S1]
#endif

/**
 * Abstract class implementing operators and methods common to all movable
 * numbers.
 *
 * @tparam ExactType The exact type of the derived class.
 * @tparam Number The underlying number type retrieved by dereferencing.
 */
template <typename ExactType, typename Number>
class movable_number {
  protected:
    /// Indicates whether the \a movable_number owns its number.
    bool owns = true;

  public:
    /// The underlying number type.
    using number_t = Number;

    /// Frees the underlying number
    virtual void destroy () = 0;

    /// Destructor calls destroy only when the number is owned
    ~movable_number () { if (owns) ((ExactType*) this)->destroy (); }

    /// Makes a deep copy of the number.
    /// The returned \a movable_number owns its number.
    static ExactType copy (const ExactType& other) { return ExactType (other); }

    /// Makes a proxy of another movable number.
    /// The returned \a movable_number does not own its number.
    static ExactType proxy (ExactType& other) { return ExactType (other, false); }

    /// Steals ownership from another movable number, if it owned.
    /// If the other number did not owned, this is the same as making a proxy.
    /// At the end, \a other will not own.
    static ExactType steal (ExactType& other) {
      bool was_owned = other.owns; // take ownership if other had it
      other.owns = false;          // ensure that other won't destroy id
      return ExactType (other, was_owned);
    }

    /// Steals ownership from \a other if it had it, otherwise makes a copy.
    /// \a other will not own after this.
    /// @return An owned \a movable_number
    static ExactType steal_or_copy (ExactType& other) {
      if (other.owns) // steal
        return steal (other);
      else
        return copy (other);
    }

    ///@{
    /** Get the referenced value.  @return The referenced value. */
    virtual number_t& get () = 0;
    virtual const number_t& get () const = 0;
    number_t& operator* () { return get (); }
    const number_t& operator* () const { return get (); }
    ///@}

    /// Same as move constructor, but may destroy current number.
    virtual ExactType& operator= (ExactType&& other) = 0;
    /// Sets the current number to another value.  No change to ownership, no destroying.
    virtual ExactType& operator= (const ExactType& other) = 0;

    ///@cond
#define ExactThis (static_cast<ExactType*> (this))      \
    ///@endcond

    ///@{
    /** Assignment operators.  Defined here as they do not conflict with the
     * implicitly defined \a operator=.  See
     * e.g. https://stackoverflow.com/questions/78903506/. */
    ExactType& operator= (ExactType& other) {
      get () = *other;
      return *ExactThis;
    }

    ExactType& operator= (int64_t other) {
      get () = other;
      return *ExactThis;
    }

    ExactType& operator= (const number_t& other) {
      get () = other;
      return *ExactThis;
    }
    ///@}

    ///@{
    /** Operators delegated to \a get. */
    bool operator== (const ExactType& other) const { return get () == *other; }
    bool operator<  (const ExactType& other) const { return get () < *other; }
    bool operator<= (const ExactType& other) const { return get () <= *other; }
    bool operator>  (const ExactType& other) const { return get () > *other; }
    bool operator>= (const ExactType& other) const { return get () >= *other; }

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

    ExactType& operator+= (const ExactType& other)   { get () += *other; return *ExactThis; }
    ExactType& operator+= (int64_t other)         { get () += other; return *ExactThis; }
    ExactType& operator+= (const number_t& other) { get () += other; return *ExactThis; }

    ExactType& operator-= (const ExactType& other)   { get () -= *other; return *ExactThis; }
    ExactType& operator-= (int64_t other)         { get () -= other; return *ExactThis; }
    ExactType& operator-= (const number_t& other) { get () -= other; return *ExactThis; }
    ///@}

#undef ExactThis
};

/// A minimalistic generic pooling allocator.
template <typename T>
class lazy_maker {
    inline static auto stash = std::vector<T> (1 << 15);
    inline static std::stack<size_t> free_Ts {};
    inline static size_t last_used = 0;
    inline static size_t used = 0;

  public:
    /// Retrieves element with id @a t.
    inline static T& get (size_t t) { return stash[t]; }

    /// Makes a new element.
    /// @return An element id, to be used in \a free and \a get.
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

    /// Releases an element.
    /// @arg t An id as returned by \a make
    static void free (size_t t) {
      if (t == last_used - 1)
        --last_used;
      else
        free_Ts.push (t);
      --used;
    }

    /// Indicates whether no element is currently allocated.
    static bool empty () {
      return used == 0;
    }
};


template <typename Number>
class lazy_number final : public movable_number<lazy_number<Number>, Number> {
  private:
    using mn_t = movable_number<lazy_number, Number>;
    friend mn_t;
    size_t id;

  private:
    lazy_number () : id (lazy_maker<number_t>::make ()) { mn_t::owns = true; }
    lazy_number (const lazy_number& other) : lazy_number () { get () = *other; } // deep
    lazy_number (const lazy_number& other, bool owns) : id (other.id) { mn_t::owns = owns; } // shallow

  public:
    using number_t = Number;
    using mn_t::operator=;

    void destroy () { assert (mn_t::owns); lazy_maker<number_t>::free (id); }

    lazy_number (lazy_number&& other) : id (other.id) {
      mn_t::owns = other.owns;
      other.owns = false;
    }

    lazy_number (const int64_t& src) : lazy_number () { get () = src; }
    lazy_number (const number_t& src) : lazy_number () { get () = src; }
    lazy_number (number_t&& src) : lazy_number () { get () = std::move (src); }

    lazy_number& operator= (lazy_number&& other) {
      if (mn_t::owns)
        destroy ();
      id = other.id;
      mn_t::owns = other.owns;
      other.owns = false;
      return *this;
    }
    lazy_number& operator= (const lazy_number& other) {
      get () = *other;
      return *this;
    }

    number_t& get () { return lazy_maker<number_t>::get (id); }
    const number_t& get () const { return lazy_maker<number_t>::get (id); }
};


template <typename T>
std::ostream& operator<< (std::ostream& os, const lazy_number<T>& wt) {
  os << *wt;
  return os;
}

template <typename Number>
class active_number final : public movable_number<active_number<Number>, Number> {
  public:
    using number_t = Number;

  private:
    using mn_t = movable_number<active_number, Number>;
    friend mn_t;
    number_t* num;

  private:
    active_number () : num (new number_t ()) { mn_t::owns = true; }
    active_number (const active_number& other) : active_number () { get () = *other; }
    active_number (const active_number& other, bool owns) : num (other.num) { mn_t::owns = owns; }

  public:
    using mn_t::operator=;

    void destroy () { assert (mn_t::owns); delete num; }

    active_number (active_number&& other) : num (other.num) {
      mn_t::owns = other.owns;
      other.owns = false;
    }

    active_number (const int64_t& src) : active_number () { get () = src; }
    active_number (const number_t& src) : active_number () { get () = src; }
    active_number (number_t&& src) : active_number () { get () = std::move (src); }

    active_number& operator= (active_number&& other) {
      if (mn_t::owns)
        destroy ();
      num = other.num;
      mn_t::owns = other.owns;
      other.owns = false;
      return *this;
    }

    active_number& operator= (const active_number& other) {
      get () = *other;
      return *this;
    }

    number_t& get () { return *num; }
    const number_t& get () const { return *num; }
};



template <typename T>
std::ostream& operator<< (std::ostream& os, const active_number<T>& wt) {
  os << *wt;
  return os;
}
