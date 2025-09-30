#pragma once
#include <type_traits>
#include <concepts>
#include <optional>
#include <functional>
#include <limits>

namespace asp::iter {

template <typename, typename>
class Map;

template <typename, typename>
class Filter;

template <typename Concrete, typename Item_>
class Iter {
public:
    using Item = Item_;

    // defined in Map.hpp
    template <typename F>
    auto map(F&& f) &&;

    // defined in Inspect.hpp
    template <typename F>
    auto inspect(F&& f) &&;

    // defined in Filter.hpp
    template <typename F>
    auto filter(F&& f) &&;

    // defined in Chain.hpp
    template <typename It2>
    auto chain(It2&& it2) &&;

    // defined in Take.hpp
    auto take(size_t n) &&;

    // defined in Zip.hpp
    template <typename It2>
    auto zip(It2&& it2) &&;

    // defined in Skip.hpp
    auto skip(size_t n) &&;

    // defined in Cycle.hpp
    auto cycle() &&;

    template <typename F>
    bool all(F&& f) && {
        while (auto item = this->derived().next()) {
            if (!std::invoke(f, *item)) {
                return false;
            }
        }

        return true;
    }

    template <typename F>
    bool any(F&& f) && {
        while (auto item = this->derived().next()) {
            if (std::invoke(f, *item)) {
                return true;
            }
        }

        return false;
    }

    template <typename F>
    std::optional<Item> find(F&& f) && {
        while (auto item = this->derived().next()) {
            if (std::invoke(f, *item)) {
                return std::move(*item);
            }
        }

        return std::nullopt;
    }

    size_t count() && {
        size_t cnt = 0;
        while (this->derived().next()) {
            ++cnt;
        }
        return cnt;
    }

    template <typename Cont>
    auto collect() && {
        Cont cont;
        while (auto item = this->derived().next()) {
            if constexpr (requires { cont.push_back(*item); }) {
                cont.push_back(*item);
            } else if constexpr (requires { cont.insert(*item); }) {
                cont.insert(*item);
            } else if constexpr (requires { cont.push(*item); }) {
                cont.push(*item);
            } else {
                static_assert(std::is_void_v<Cont>, "Container must support push_back, insert or push");
            }
        }
        return cont;
    }

    auto enumerate() && {
        return std::move(*this).map([n = size_t{0}](auto item) mutable {
            return std::make_pair(n++, std::move(item));
        });
    }

    void forEach(auto&& f) && {
        while (auto item = this->derived().next()) {
            std::invoke(f, *item);
        }
    }

    std::optional<Item> min() && requires(requires (Item x, Item y) { x < y; }){
        std::optional<Item> minItem;
        while (auto item = this->derived().next()) {
            if (!minItem || std::less<Item>{}(*item, *minItem)) {
                minItem = item;
            }
        }
        return minItem;
    }

    std::optional<Item> max() && requires(requires (Item x, Item y) { x < y; }) {
        std::optional<Item> maxItem;
        while (auto item = this->derived().next()) {
            if (!maxItem || std::less<Item>{}(*maxItem, *item)) {
                maxItem = item;
            }
        }
        return maxItem;
    }

    auto sum() && requires(requires { Item{} + Item{}; }) {
        Item total{};
        while (auto item = this->derived().next()) {
            total += *item;
        }
        return total;
    }

private:
    Concrete& derived() {
        return static_cast<Concrete&>(*this);
    }
};

// Wrapper for C++ iterators

template <typename It>
concept IsCxxIterator = requires {
    { std::declval<It>() != std::declval<It>() } -> std::convertible_to<bool>;
    { ++std::declval<It&>() } -> std::same_as<It&>;
    { *std::declval<It>() };
};

template <IsCxxIterator It>
class CxxIter : public Iter<CxxIter<It>, std::remove_reference_t<decltype(*std::declval<It>())>> {
public:
    using Item = CxxIter::Item;

    CxxIter(It begin, It end) : m_current(begin), m_end(end) {};

    std::optional<Item> next() {
        if (m_current == m_end) {
            return std::nullopt;
        }

        auto item = std::move(*m_current);
        ++m_current;
        return item;
    }

private:
    It m_current, m_end;
};

// Empty iterator

template <typename It = void**>
auto empty() {
    return CxxIter<It>(It{}, It{});
}

// One shot iterator

template <typename T>
class OnceIter : public Iter<OnceIter<T>, T> {
public:
    using Item = T;

    OnceIter(std::optional<T> value) : m_value(std::move(value)) {}

    std::optional<T> next() {
        if (m_value) {
            auto item = std::move(*m_value);
            m_value = std::nullopt;
            return item;
        }

        return std::nullopt;
    }

private:
    std::optional<T> m_value;
};

template <typename T>
auto once(T value) {
    return OnceIter<T>(std::move(value));
}

// Repeating iterator

template <typename T>
class RepeatIter : public Iter<RepeatIter<T>, T> {
public:
    using Item = T;

    RepeatIter(T value, size_t times) : m_value(std::move(value)), m_times(times) {}

    std::optional<T> next() {
        if (m_times == 0) {
            return std::nullopt;
        }

        --m_times;
        return m_value;
    }

private:
    T m_value;
    size_t m_times;
};

template <typename T>
auto repeat(T value, size_t times) {
    return RepeatIter<T>(std::move(value), times);
}

template <typename T>
auto repeat(T value) {
    return RepeatIter<T>(std::move(value), std::numeric_limits<size_t>::max());
}

}
