#pragma once
#include <optional>
#include <cstddef>

namespace asp::iter {

template <typename C>
struct Collector {
    static constexpr bool CollectorUnmplemented = true;

    // Interface to implement

    Collector(std::optional<size_t> sizeHint) {};

    template <typename T>
    void push(T&& item) {}

    C finish() {
        static_assert(!CollectorUnmplemented);
        return C{};
    }
};

template <typename C>
struct Collector2 {
    static constexpr bool Collector2Unmplemented = true;

    // Interface to implement

    Collector2(std::optional<size_t> sizeHint) {};

    template <typename T, typename Y>
    void push(T&& item1, Y&& item2) {}

    C finish() {
        static_assert(!Collector2Unmplemented);
        return C{};
    }
};

template <typename C>
concept IsCollector = !requires() { Collector<C>::CollectorUnmplemented; };

template <typename C>
concept IsCollector2 = !requires() { Collector2<C>::Collector2Unmplemented; };

}