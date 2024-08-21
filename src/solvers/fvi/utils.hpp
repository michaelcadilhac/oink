#pragma once

template <typename T, template <typename...> class TT>
constexpr bool is_instantiation_of_v = false;

template <template <typename...> class TT, typename... TS>
constexpr bool is_instantiation_of_v <TT<TS...>, TT> = true;

template <class C, template<typename...> class TT>
concept instantiation_of = is_instantiation_of_v<C, TT>;

using vertex_t = int32_t;
using priority_t =  decltype(std::declval<pg::Game> ().priority (vertex_t ()));
using logger_t = std::ostream;

#define log(T) do { if (this->trace >= 1) { this->logger << T; } } while (0)
#define log_stat(T) do { std::cout << T; } while (0)
