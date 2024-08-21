#pragma once

using gmp_t = boost::multiprecision::mpz_int;

template <std::same_as<gmp_t> W>
bool set_if_plus_larger (W& orig, const W& p1, const W& p2) {
  static W tmp = 0; // do not merge with next line, this is a static!
  tmp = p1;
  tmp += p2;
  if (orig < tmp) {
    orig = tmp;
    return true;
  }
  return false;
}

template <std::same_as<gmp_t> W>
bool set_if_plus_smaller (W& orig, const W& p1, const W& p2) {
  static W tmp = 0; // do not merge with next line, this is a static!
  tmp = p1;
  tmp += p2;
  if (orig > tmp) {
    orig = tmp;
    return true;
  }
  return false;
}


template <std::same_as<gmp_t> W>
W priority_to_weight (const priority_t& prio,
                      const pg::Game& pgame,
                      bool swap)
{
  ssize_t t = pgame.nodecount ();
  ssize_t sbase = 1;
  while (t != 0) {
    t >>= 1;
    sbase <<= 1;
  }

  static W base = -1 * sbase,
    fact = pgame.nodecount () + 1;
  static const pg::Game* ppgame = &pgame;

  if (&pgame != ppgame) {
    base = -1 * (ssize_t) pgame.nodecount (),
      fact = pgame.nodecount () + 1;
    ppgame = &pgame;
  }

  if (swap)
    return W (-(pow (base, prio))); // TODO change for base^prio
  else
    return W (pow (base, prio) );
}

template <std::same_as<gmp_t> W>
W infinity_weight (const pg::Game& pgame) {
  ssize_t t = pgame.nodecount ();
  ssize_t sbase = 1;
  while (t != 0) {
    t >>= 1;
    sbase <<= 1;
  }
  // TODO Change for pow (W (pgame.nodecount ()), pgame.priority (pgame.nodecount () - 1)) * (pgame.nodecount () + 1))
  return W (((pow (W (sbase), (pgame.priority (pgame.nodecount () - 1) + 1)))));

  // return W (((pow (W (pgame.nodecount ()), pgame.priority (pgame.nodecount () - 1)) *
  //             (pgame.nodecount () + 1))));
    //- 1) *
    //      (pgame.nodecount () - 1) + 1);
}

template <std::same_as<gmp_t> W>
W zero_weight (const W&) {
  return W (0);
}
