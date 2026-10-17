#pragma once
#include <type_traits>
#include <utility>
#include <functional>

namespace asp::iter {

namespace detail {
    template <typename T>
    struct IsReferenceWrapper : std::false_type {};

    template <typename U>
    struct IsReferenceWrapper<std::reference_wrapper<U>> : std::true_type {};

    template <typename T>
    struct IsPair : std::false_type {};

    template <typename U, typename V>
    struct IsPair<std::pair<U, V>> : std::true_type {};

    template <typename T>
    struct ExtractRefWrapper {
        using type = T;
    };

    template <typename T>
    struct ExtractRefWrapper<std::reference_wrapper<T>> {
        using type = T;
    };

    template <typename T>
    struct UnwrapRefWrapper {
        using type = T;
    };

    template <typename T>
    struct UnwrapRefWrapper<std::reference_wrapper<T>> {
        using type = T&;
    };

    template <typename T>
    struct WrapRefWrapper {
        using type = T;
    };

    template <typename T>
    struct WrapRefWrapper<T&> {
        using type = std::reference_wrapper<T>;
    };

    template <typename T>
    struct WrapRefWrapper<T&&> {
        using type = T;
    };
}

template <typename T>
static constexpr bool IsReferenceWrapper = detail::IsReferenceWrapper<T>::value;
template <typename T>
static constexpr bool IsPair = detail::IsPair<T>::value;

template <typename T>
using ExtractRefWrapper = typename detail::ExtractRefWrapper<T>::type;
template <typename T>
using UnwrapRefWrapper = typename detail::UnwrapRefWrapper<T>::type;

// Wraps into a reference_wrapper only if T is an lvalue reference
template <typename T>
using WrapRefWrapper = typename detail::WrapRefWrapper<T>::type;

template <typename Y, typename T = std::decay_t<Y>>
UnwrapRefWrapper<T> unwrapRefWrapper(Y&& value) {
    if constexpr (IsReferenceWrapper<T>) {
        return value.get();
    } else {
        return value;
    }
}

template <typename T>
WrapRefWrapper<T> wrapRefWrapper(T&& value) {
    if constexpr (std::is_lvalue_reference_v<T>) {
        return std::ref(value);
    } else {
        return std::forward<T>(value);
    }
}

}
