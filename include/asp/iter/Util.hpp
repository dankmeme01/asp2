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

}
