#pragma once


#include <concepts>
#include <cstdint>

#ifndef NOINK
/// Game types
using vertex_t = int32_t;
using priority_t =  decltype(std::declval<pg::Game> ().priority (vertex_t ()));
using logger_t = std::ostream;

template <typename T>
T priority_to_number (const priority_t& prio,
                      const pg::Game& pgame,
                      bool swap);

template <typename T>
T infinity_number (const pg::Game& pgame);

#else
#include <boost/multiprecision/gmp.hpp>
/// Game types
using vertex_t = int32_t;
using priority_t = boost::multiprecision::mpz_int;
using logger_t = std::ostream;
#endif

/// Number

template <typename T>
bool set_if_plus_larger (T& w, const T& w1, const T& w2);

template <typename T>
bool set_if_plus_smaller (T& w, const T& w1, const T& w2);

template <typename T>
T zero_number (const T& t);

template <typename W>
concept Number =
  std::is_constructible_v<W, W&&> and
  std::is_constructible_v<W, const W&> and
  std::is_default_constructible_v<W> and
  requires (W& w, const W& cw, std::ostream& os) {
    { set_if_plus_larger (w, cw, cw) } -> std::same_as<bool>;
    { set_if_plus_smaller (w, cw, cw) } -> std::same_as<bool>;
    w -= w;
    w += w;
    w == -w;
    cw < 0 == cw > 0;
  } and
#ifndef NOINK
  requires (W& w, const pg::Game& pgame) {
    w = priority_to_number<W> (0, pgame, true);
    w = infinity_number<W> (pgame);
    w = zero_number (w);
  }
#else
  true
#endif
  ;

/// Allocator

template <typename A, typename O>
concept Allocator = std::is_constructible_v<A, uint64_t> and
  requires (A& a, O* po) {
    { a.construct () } -> std::same_as<O*>;
    a.destroy (po);
  };


/// MovableNumber

// Helper
template <typename T, template <typename...> class TT>
constexpr bool is_instantiation_of_v = false;

template <template <typename...> class TT, typename... TS>
constexpr bool is_instantiation_of_v <TT<TS...>, TT> = true;

template <class C, template<typename...> class TT>
concept instantiation_of = is_instantiation_of_v<C, TT>;

template <Number N, Allocator<N>>
class movable_number;

template <typename W>
concept MovableNumber = is_instantiation_of_v<W, movable_number> and Number<typename W::number_t>;
