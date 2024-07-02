#pragma once

#include <type_traits>
#include <vector>
#include <optional>
#include <array>

namespace asp {
    template <typename T, typename... Args>
    constexpr bool is_one_of = std::disjunction_v<std::is_same<T, Args>...>;

    /* member_ptr_to_underlying */

    template <typename>
    struct member_ptr_to_underlying;

    template <typename R, typename C>
    struct member_ptr_to_underlying<R C::*> {
        using type = R;
    };

    template <typename R, typename C>
    struct member_ptr_to_underlying<R C::* const> {
        using type = R;
    };

    template <typename R, typename C>
    struct member_ptr_to_underlying<R C::* volatile> {
        using type = R;
    };

    template <typename R, typename C>
    struct member_ptr_to_underlying<R C::* const volatile> {
        using type = R;
    };


    template <typename>
    struct is_std_vector : std::false_type {};

    template<typename T, typename A>
    struct is_std_vector<std::vector<T, A>> : std::true_type {};

    template <typename>
    struct is_std_pair : std::false_type {};

    template <typename T1, typename T2>
    struct is_std_pair<std::pair<T1, T2>> : std::true_type {};

    template <typename>
    struct is_std_array : std::false_type {};

    template <typename T, size_t N>
    struct is_std_array<std::array<T, N>> : std::true_type {};

    template <typename>
    struct is_std_optional : std::false_type {};

    template <typename T>
    struct is_std_optional<std::optional<T>> : std::true_type {};

}