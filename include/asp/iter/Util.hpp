#pragma once
#include <type_traits>

namespace asp::iter {

template <typename T>
struct is_reference_wrapper : std::false_type {};

template <typename U>
struct is_reference_wrapper<std::reference_wrapper<U>> : std::true_type {};

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

template <typename Y, typename T = std::decay_t<Y>>
UnwrapRefWrapper<T>::type unwrapRefWrapper(Y&& value) {
    if constexpr (is_reference_wrapper<T>::value) {
        return value.get();
    } else {
        return value;
    }
}

}
