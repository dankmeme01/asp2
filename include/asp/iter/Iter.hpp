#pragma once
#include <asp/detail/config.hpp>
#include <type_traits>
#include <concepts>
#include <optional>
#include <functional>
#include <limits>
#include <string>
#include <iterator>
#include <array>
#include <fmt/format.h>
#include "Util.hpp"

namespace asp::iter {

struct IterBase {};

/// The base template for all iterators, which has many transformational and terminal methods and utilities.
/// Every iterator instance can also be iterated over using a plain C++ range-based for loop.
/// The main method of consuming items is `std::optional<Item> next()`, and that's what you should implement in your own iterators.
///
/// Intermediate operations are *lazy*, meaning they do not do any work until an actual terminal operation is called,
/// such as `count()`, `max()`, `collect()`, etc. This means a `map()` call does not immediately map any element, only the final terminal operation does.
template <typename Concrete, typename Item_>
class Iter : public IterBase {
public:
    using Item = Item_;

    /// Returns an iterator that invokes the given function on every item and returns the result, mapping the items.
    template <typename F>
    auto map(F&& f) &&; // defined in Map.hpp

    /// Returns an iterator that copies data from references into owned objects.
    /// For example, if this is an iterator of `std::string&` or `reference_wrapper<std::string>`,
    /// `copied()` will return an iterator that produces `std::string` by copying the referenced object.
    auto copied() &&; // defined in Copied.hpp

    /// Returns the iterator that invokes the given function on every item before yielding that item again.
    /// Note that this is not a terminal operation, so the function will not be eagerly invoked.
    template <typename F>
    auto inspect(F&& f) &&; // defined in Inspect.hpp

    /// Returns an iterator that only yields items for which the given predicate returns true.
    template <typename F>
    auto filter(F&& f) &&; // defined in Filter.hpp

    /// Similar to filter, but using `std::optional`: the provided function takes an item and
    /// must return an `std::optional<T>`. If the function returns `std::nullopt`, the item is skipped.
    template <typename F>
    auto filterMap(F&& f) &&; // defined in FilterMap.hpp

    /// Returns a concatenation of this iterator and `it2`.
    /// It will yield elements of this iterator until they run out, before switching to `it2`.
    template <typename It2>
    auto chain(It2&& it2) &&; // defined in Chain.hpp

    /// Limits how many items can be yielded by this iterator.
    /// The iterator will not yield any more items after `n` items have been yielded.
    auto take(size_t n) &&; // defined in Take.hpp

    /// Returns an iterator that merges the elements yielded by this iterator and the provided iterator `it2`.
    /// The resulting iterator will yield `std::pair<T, Y>` where `T` is the item type of this iterator and `Y` is the item type of `it2`.
    /// Stops yielding items once any of the two iterator runs out.
    template <typename It2>
    auto zip(It2&& it2) &&; // defined in Zip.hpp

    /// Returns an iterator that skips yielding the first `n` items of this iterator.
    auto skip(size_t n) &&; // defined in Skip.hpp

    /// Returns an iterator that yields the items of this iterator in a cycle, repeating indefinitely.
    auto cycle() &&; // defined in Cycle.hpp

    /// Returns an iterator that groups items of this iterator into statically sized chunks of size `N`.
    /// For example, if this iterator has 10 items and `arrayChunks<3>()` is called,
    /// the resulting iterator will yield `std::array<T, 3>` three times before being exhausted.
    ///
    /// If the amount of items in this iterator is not divisible by `N`, the last few items will be dropped.
    template <size_t N>
    auto arrayChunks() &&; // defined in ArrayChunks.hpp

    /// Returns the next chunk of N items from the iterator, or `std::nullopt` if there are not enough items left to fill the chunk.
    /// Note: while not a terminal operation, this always consumes at most N items. For example if an iterator has 2 items left and nextChunk<3> is called,
    /// the 2 items will be consumed and the iterator will be exhausted, while returning `std::nullopt`.
    template <size_t N>
    std::optional<std::array<Item, N>> nextChunk() {
        static_assert(N > 0, "N must be greater than 0 for nextChunk");

        std::array<Item, N> chunk;
        for (size_t i = 0; i < N; ++i) {
            auto item = this->derived().next();
            if (!item) {
                return std::nullopt;
            }
            chunk[i] = std::move(*item);
        }
        return chunk;
    }

    /// Returns an iterator that yields the elements of the nested iterators.
    /// For example, if this is an iterator of `std::vector<int>`, then the resulting iterator
    /// will first yield integers from the first vector, then the next, and so on.
    /// Only valid with iterators that yield containers or other iterators.
    auto flatten() &&; // defined in Flatten.hpp

    /// Shorthand for `map(f).flatten()`, this allows you to map each item to a container or iterator,
    /// and have the resulting iterator produce the underlying items of those containers/iterators.
    template <typename F>
    auto flatMap(F&& f) && {
        return std::move(*this).map(std::forward<F>(f)).flatten();
    }

    /// Returns whether all items in this iterator match the given predicate, or `true` if there are no items.
    template <typename F>
    bool all(F&& f) && {
        while (auto item = this->derived().next()) {
            if (!std::invoke(f, unwrapRefWrapper(*item))) {
                return false;
            }
        }

        return true;
    }

    /// Returns whether any item in this iterator matches the given predicate, or `false` if there are no items.
    template <typename F>
    bool any(F&& f) && {
        while (auto item = this->derived().next()) {
            if (std::invoke(f, unwrapRefWrapper(*item))) {
                return true;
            }
        }

        return false;
    }

    /// Returns the first item that matches the given predicate, or `std::nullopt` if no item matches.
    template <typename F>
    std::optional<Item> find(F&& f) && {
        while (auto item = this->derived().next()) {
            if (std::invoke(f, unwrapRefWrapper(*item))) {
                return std::move(*item);
            }
        }

        return std::nullopt;
    }

    /// Returns the nth item in the iterator, or null if the iterator gets exhausted before reaching it.
    /// `n` is zero indexed, so `nth(0)` returns the very next item, equivalent to calling `next()`.
    std::optional<Item> nth(size_t n) && {
        for (size_t i = 0; i < n; ++i) {
            if (!this->derived().next()) {
                return std::nullopt;
            }
        }

        return this->derived().next();
    }

    /// Consumes the iterator and returns the number of items yielded.
    size_t count() && {
        size_t cnt = 0;
        while (this->derived().next()) {
            ++cnt;
        }
        return cnt;
    }

    /// Collects the items of this iterator into the provided collection, by default `std::vector<T>`.
    /// The collection must support one of the methods: `push_back(T)`, `insert(T)` or `push(T)`.
    template <typename Cont = std::vector<Item>>
    auto collect() && {
        // TODO: use some custom collector interface
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

    /// Joins the items of this iterator into a string, using the provided separator between items.
    /// The items must be formattable with `fmt`, for example by defining `format_as(const T&) -> std::string_view`
    std::string join(std::string_view separator) && {
        std::string result;
        bool first = true;
        while (auto item = this->derived().next()) {
            if (!first) {
                result += separator;
            } else {
                first = false;
            }
            fmt::format_to(std::back_inserter(result), "{}", *item);
        }
        return result;
    }

    /// Returns an iterator that yields pairs of (index, item) for each item in this iterator.
    auto enumerate() && {
        return std::move(*this).map([n = size_t{0}](auto item) mutable {
            return std::make_pair(n++, std::move(item));
        });
    }

    /// Folds every element into an accumulator by applying an operation, and returns the final result.
    /// A mutable reference to the `init` argument will be passed as the first argument to the operation, together with the item.
    template <typename B, typename F>
    B fold(B init, F&& f) && {
        while (auto item = this->derived().next()) {
            std::invoke(f, init, unwrapRefWrapper(*item));
        }
        return init;
    }

    /// Shorthand for `map()` with a `static_cast` inside, casts each item to the given type `Y`.
    template <typename Y>
    auto mapCast() && {
        return std::move(*this).map([](auto item) {
            return static_cast<Y>(item);
        });
    }

    /// Consumes the iterator and invokes the given function on every item.
    void forEach(auto&& f) && {
        while (auto item = this->derived().next()) {
            std::invoke(f, unwrapRefWrapper(*item));
        }
    }

    /// Consumes the iterator and returns the minimum item, or `std::nullopt` if there are no items.
    std::optional<Item> min() && requires(requires (Item x, Item y) { x < y; }){
        std::optional<Item> minItem;
        while (auto item = this->derived().next()) {
            if (!minItem || std::less<Item>{}(*item, *minItem)) {
                minItem = item;
            }
        }
        return minItem;
    }

    /// Consumes the iterator and returns the maximum item, or `std::nullopt` if there are no items.
    std::optional<Item> max() && requires(requires (Item x, Item y) { x < y; }) {
        std::optional<Item> maxItem;
        while (auto item = this->derived().next()) {
            if (!maxItem || std::less<Item>{}(*maxItem, *item)) {
                maxItem = item;
            }
        }
        return maxItem;
    }

    /// Consumes the iterator and returns the sum of all items, or `Item{}` if there are no items.
    auto sum() && requires(requires { Item{} + Item{}; }) {
        Item total{};
        while (auto item = this->derived().next()) {
            total += *item;
        }
        return total;
    }

    // c++ iterator support
    struct iterator {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = Item;
        using difference_type = std::ptrdiff_t;
        using pointer = Item*;
        using reference = Item&;

        iterator(Iter& iter, bool end) : m_iter(iter) {
            if (!end) {
                this->getNext();
            }
        }

        iterator& operator++() {
            ASP_ASSERT(m_item, "Incrementing end iterator");
            this->getNext();
            return *this;
        }

        reference operator*() {
            ASP_ASSERT(m_item, "Dereferencing end iterator");
            return *m_item;
        }

        bool operator==(const iterator& other) const {
            return &m_iter == &other.m_iter && m_item.has_value() == other.m_item.has_value();
        }

        bool operator!=(const iterator& other) const {
            return !(*this == other);
        }

    private:
        std::optional<Item> m_item;
        Iter& m_iter;

        void getNext() {
            // we do this silly stuff instead of a simple equals assignment,
            // because we want to ensure we create a *new* Item object instead of calling operator= on the previous one.
            // this is crucial if Item is e.g. pair<T, Y&>, we want to rebind instead of moving the object into the one pointed to by Y&.
            auto next = m_iter.derived().next();
            if (next) {
                m_item.emplace(std::move(*next));
            } else {
                m_item = std::nullopt;
            }
        }
    };

    auto begin() {
        return iterator{*this, false};
    }

    auto end() {
        return iterator{*this, true};
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

template <typename InpT, typename T = decltype(*std::declval<InpT>())>
using CxxIterUntrivialize = std::conditional_t<
    std::is_trivially_copyable_v<std::remove_reference_t<T>>,
    std::decay_t<T>,
    T
>;

template <typename InpT, typename T = CxxIterUntrivialize<InpT>>
using CxxIterUnderlying = std::conditional_t<
    std::is_reference_v<T>,
    std::reference_wrapper<std::remove_reference_t<T>>,
    std::remove_cv_t<T>
>;

template <IsCxxIterator It>
class CxxIter : public Iter<CxxIter<It>, CxxIterUnderlying<It>> {
public:
    using Item = CxxIterUnderlying<It>;
    constexpr static bool IsRef = std::is_reference_v<CxxIterUntrivialize<It>>;

    CxxIter(It begin, It end) : m_current(begin), m_end(end) {};

    std::optional<Item> next() {
        if (m_current == m_end) {
            return std::nullopt;
        }

        if constexpr (IsRef) {
            auto rw = std::ref(*m_current);
            ++m_current;
            return rw;
        } else {
            decltype(auto) item = *m_current;
            ++m_current;
            return item;
        }
    }

private:
    It m_current, m_end;
};

template <IsCxxIterator It>
class CxxIterConsume : public Iter<CxxIterConsume<It>, std::decay_t<decltype(*std::declval<It>())>> {
public:
    using Item = std::decay_t<decltype(*std::declval<It>())>;

    CxxIterConsume(It begin, It end) : m_current(begin), m_end(end) {};

    std::optional<Item> next() {
        if (m_current == m_end) {
            return std::nullopt;
        }

        Item item = std::move(*m_current);
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
