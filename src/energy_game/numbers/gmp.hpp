#pragma once

#include <boost/multiprecision/gmp.hpp>

class gmp;

/// This is fairly deep into the boost implementation.  We're saying: 1. @a gmp
/// is not immediately comparable with a Boost number: this forces a conversion,
/// and resolves templates correctly, and 2. the "canonical" of gmp is the
/// backend of mpz_int (i.e., gmp_int).  See the definition of @a
/// canonical_value in @a number.hpp in Boost.
namespace boost::multiprecision::detail {
  template <class B, expression_template_option ET>
  struct is_valid_mixed_compare<number<B, ET>, gmp> : public std::integral_constant<bool, false>
  {};

  template <class B, expression_template_option ET>
  struct is_valid_mixed_compare<gmp, number<B, ET>> : public std::integral_constant<bool, false>
  {};

  template <typename Backend, typename Tag>
  struct canonical_imp<gmp, Backend, Tag> { using type = gmp_int; };
}

class gmp : public boost::multiprecision::mpz_int {
    using gmp_t = boost::multiprecision::mpz_int;
    using gmp_int = boost::multiprecision::backends::gmp_int;
  public:
    using gmp_t::gmp_t;

    operator gmp_int& () { return this->backend (); }
    operator const gmp_int& () const { return this->backend (); }

    gmp& operator+= (const auto& other) {
      gmp_t::operator+= (static_cast<const gmp_t&> (other));
      return *this;
    }

    bool set_if_plus_larger (const gmp& p1, const gmp& p2) {
      static gmp tmp = 0; // do not merge with next line, this is a static!
      tmp = p1;
      tmp += p2;
      if (static_cast<gmp_t> (*this) < static_cast<gmp_t> (tmp)) {
        *this = tmp;
        return true;
      }
      return false;
    }

    bool set_if_plus_smaller (const gmp& p1, const gmp& p2) {
      static gmp tmp = 0; // do not merge with next line, this is a static!
      tmp = p1;
      tmp += p2;
      if (static_cast<gmp_t> (*this) > static_cast<gmp_t> (tmp)) {
        *this = tmp;
        return true;
      }
      return false;
    }

    static gmp priority_to_number (const priority_t& prio,
                                     const pg::Game& pgame,
                                     bool swap)
    {
      // TODO Clean this nonsense.
      ssize_t t = pgame.nodecount ();
      ssize_t sbase = 1;
      while (t != 0) {
        t >>= 1;
        sbase <<= 1;
      }

      static gmp base = -1 * sbase,
        fact = pgame.nodecount () + 1;
      static const pg::Game* ppgame = &pgame;

      if (&pgame != ppgame) {
        base = -1 * (ssize_t) pgame.nodecount (),
          fact = pgame.nodecount () + 1;
        ppgame = &pgame;
      }

      if (swap)
        return gmp (-(pow (base, prio))); // TODO change for base^prio
      else
        return gmp (pow (base, prio) );
    }

    static gmp infinity_number (const pg::Game& pgame) {
      ssize_t t = pgame.nodecount ();
      ssize_t sbase = 1;
      while (t != 0) {
        t >>= 1;
        sbase <<= 1;
      }
      // TODO Change for pow (gmp (pgame.nodecount ()), pgame.priority (pgame.nodecount () - 1)) * (pgame.nodecount () + 1))
      return gmp (((pow (gmp (sbase), (pgame.priority (pgame.nodecount () - 1) + 1)))));

      // return gmp (((pow (gmp (pgame.nodecount ()), pgame.priority (pgame.nodecount () - 1)) *
      //             (pgame.nodecount () + 1))));
      //- 1) *
      //      (pgame.nodecount () - 1) + 1);
    }

    static gmp zero_number (const gmp&) {
      return gmp (0);
    }

    std::ostream& print (std::ostream& os) const { return os << *static_cast<const gmp_t*> (this); }
};

/*
  static_assert (std::is_convertible_v<boost::multiprecision::mpz_int, boost::multiprecision::backends::gmp_int>);
  static_assert (std::is_convertible_v<gmp, boost::multiprecision::mpz_int>);
  static_assert (std::is_convertible_v<gmp, boost::multiprecision::backends::gmp_int>);
 */
