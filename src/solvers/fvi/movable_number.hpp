#pragma once

/**
 * @file movable_number.hpp
 * @brief A type of reference to numbers where the reference can be owned
 * or not owned.  Allows copying (owned), stealing (owned if the source was),
 * proxying (not owned), and steal-or-copying (owned, stolen if the other is
 * owned, copied otherwise).
 */

#include <solvers/fvi/allocators.hpp>

/**
 * Abstract class implementing operators and methods common to all movable
 * numbers.
 *
 * @tparam Number The underlying number type retrieved by dereferencing.
 */
template <typename Number, typename Allocator = new_delete_allocator<Number>>
class movable_number {
  public:
    /// The underlying number type.
    using number_t = Number;

    movable_number () : num (allocator.construct ()) { owns = true; }
    movable_number (const movable_number& other) : movable_number () { *num = *other; } // deep
    movable_number (const movable_number& other, bool owns) : num (other.num), owns (owns) {} // shallow
    movable_number (movable_number&& other) : num (other.num) {
      owns = other.owns;
      other.owns = false;
    }

    movable_number (const int64_t& src) : movable_number () { *num = src; }
    movable_number (const number_t& src) : movable_number () { *num = src; }
    movable_number (number_t&& src) : movable_number () { *num = std::move (src); }

    /// Destructor calls destroy only when the number is owned
    ~movable_number () { if (owns) allocator.destroy (num); }

    /// Makes a deep copy of the number.
    /// The returned \a movable_number owns its number.
    static movable_number copy (const movable_number& other) { return movable_number (other); }

    /// Makes a proxy of another movable number.
    /// The returned \a movable_number does not own its number.
    static movable_number proxy (movable_number& other) { return movable_number (other, false); }

    /// Steals ownership from another movable number, if it owned.
    /// If the other number did not owned, this is the same as making a proxy.
    /// At the end, \a other will not own.
    static movable_number steal (movable_number& other) {
      bool was_owned = other.owns; // take ownership if other had it
      other.owns = false;          // ensure that other won't destroy id
      return movable_number (other, was_owned);
    }

    /// Steals ownership from \a other if it had it, otherwise makes a copy.
    /// \a other will not own after this.
    /// @return An owned \a movable_number
    static movable_number steal_or_copy (movable_number& other) {
      if (other.owns) // steal
        return steal (other);
      else
        return copy (other);
    }

    ///@{
    /** Get the referenced value.  @return The referenced value. */
    number_t& operator* () { return *num; }
    const number_t& operator* () const { return *num; }
    ///@}

    ///@{
    /** Assignment operators. */

    /// Same as move constructor, but may destroy current number.
    movable_number& operator= (movable_number&& other) {
      if (owns and num != other.num)
        allocator.destroy (num);
      num = other.num;
      owns = other.owns;
      other.owns = false;
      return *this;
    }

    /// Sets the current number to another value.  No change to ownership, no destroying.
    movable_number& operator= (const movable_number& other) {
      *num = *other;
      return *this;
    }

    movable_number& operator= (int64_t other) {
      *num = other;
      return *this;
    }

    movable_number& operator= (const number_t& other) {
      *num = other;
      return *this;
    }
    ///@}

    ///@{
    /** Operators delegated to \a get. */
    bool operator== (const movable_number& other) const { return *num == *other; }
    bool operator<  (const movable_number& other) const { return *num < *other; }
    bool operator<= (const movable_number& other) const { return *num <= *other; }
    bool operator>  (const movable_number& other) const { return *num > *other; }
    bool operator>= (const movable_number& other) const { return *num >= *other; }

    bool operator== (int64_t other) const { return *num == other; }
    bool operator<  (int64_t other) const { return *num < other; }
    bool operator<= (int64_t other) const { return *num <= other; }
    bool operator>  (int64_t other) const { return *num > other; }
    bool operator>= (int64_t other) const { return *num >= other; }

    bool operator== (const number_t& other) const { return *num == other; }
    bool operator<  (const number_t& other) const { return *num < other; }
    bool operator<= (const number_t& other) const { return *num <= other; }
    bool operator>  (const number_t& other) const { return *num > other; }
    bool operator>= (const number_t& other) const { return *num >= other; }

    movable_number& operator+= (const movable_number& other)   { *num += *other; return *this; }
    movable_number& operator+= (int64_t other)         { *num += other; return *this; }
    movable_number& operator+= (const number_t& other) { *num += other; return *this; }

    movable_number& operator-= (const movable_number& other)   { *num -= *other; return *this; }
    movable_number& operator-= (int64_t other)         { *num -= other; return *this; }
    movable_number& operator-= (const number_t& other) { *num -= other; return *this; }
    ///@}

  private:
    static inline Allocator allocator { 1 << 15 };

    /// The number.
    number_t* num;

    /// Indicates whether the \a movable_number owns its number.
    bool owns = true;
};

template <typename T>
using boost_movable_number = movable_number<T, boost_allocator<T>>;

template <typename T>
using recycling_movable_number = movable_number<T, recycling_allocator<T>>;

namespace std {
  template <typename T, typename A>
  std::ostream& operator<< (std::ostream& os, const movable_number<T, A>& wt) {
    os << *wt;
    return os;
  }
}
