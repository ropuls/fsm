#pragma once

// see

#include <tuple>
#include <utility>
#include <type_traits>
#include <cstdio>

namespace detail {
    template <template <class...> class TT, template <class...> class UU, class... Us>
    auto pack(UU<Us...>)
    -> std::tuple<TT<Us>...>;

    template <template <class...> class TT, class... Ts>
    auto unpack(std::tuple<TT<Ts>...>)
    -> TT<Ts...>;

    template <std::size_t N, class T>
    using TET = std::tuple_element_t<N, T>;

    template <std::size_t N, class T, std::size_t... Is>
    constexpr auto remove_duplicates_pack_first(T, std::index_sequence<Is...>)
    -> std::conditional_t<(... || (N > Is && std::is_same_v<TET<N, T>, TET<Is, T>>)), std::tuple<>, std::tuple<TET<N, T>>>;

    template <template <class...> class TT, class... Ts, std::size_t... Is>
    auto remove_duplicates(std::tuple<TT<Ts>...> t, std::index_sequence<Is...> is)
    -> decltype(std::tuple_cat(remove_duplicates_pack_first<Is>(t, is)...));

    template <template <class...> class TT, class... Ts>
    auto remove_duplicates(TT<Ts...> t)
    -> decltype(unpack<TT>(remove_duplicates<TT>(pack<TT>(t), std::make_index_sequence<sizeof...(Ts)>())));
}

template <template <class...> class TT, class... Ts>
using merge_t = decltype(detail::unpack<TT>(std::tuple_cat(detail::pack<TT>(std::declval<Ts>())...)));

template <class T>
using remove_duplicates_t = decltype(detail::remove_duplicates(std::declval<T>()));
