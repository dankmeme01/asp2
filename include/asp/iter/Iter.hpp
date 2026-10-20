#pragma once
#include <asp/detail/config.hpp>
#include <type_traits>
#include <concepts>
#include <optional>
#include <functional>
#include <vector>
#include <string>
#include <iterator>
#include <array>
#include <utility>
#include <fmt/format.h>
#include "CollectorBase.hpp"
#include "Util.hpp"

namespace asp::iter {

template <typename Accum, typename Item>
concept Summable = requires(Accum a, Item b) {
    { a += b } -> std::same_as<Accum&>;
    { a + b } -> std::same_as<Accum>;
};

template <typename It>
concept ProvidesSizeHint = requires(const It it) {
    { it.sizeHint() } -> std::convertible_to<size_t>;
};

struct IterBase {};

/// The base template for all iterators, which has many transformational and terminal methods and utilities.
/// Every iterator instance can also be iterated over using a plain C++ range-based for loop.
/// The main method of consuming items is `std::optional<Item> next()`, and that's what you should implement in your own iterators.
///
/// Intermediate operations are *lazy*, meaning they do not do any work until an actual terminal operation is called,
/// such as `count()`, `max()`, `collect()`, etc. This means a `map()` call does not immediately map any element, only the final terminal operation does.
template <typename Concrete, typename Item_>
class ASP_ITER_NODISCARD Iter : public IterBase {
public:
    using Item = Item_;

    /// Returns an iterator that invokes the given function on every item and returns the result, mapping the items.
    template <typename F> requires std::invocable<F, UnwrapRefWrapper<Item_>>
    auto map(F&& f) &&; // defined in impl/Map.hpp

    /// Returns an iterator that copies data from references into owned objects.
    /// For example, if this is an iterator of `std::string&` or `reference_wrapper<std::string>`,
    /// `copied()` will return an iterator that produces `std::string` by copying the referenced object.
    auto copied() &&; // defined in impl/Copied.hpp

    /// Returns the iterator that invokes the given function on every item before yielding that item again.
    /// Note that this is not a terminal operation, so the function will not be eagerly invoked.
    template <typename F> requires std::invocable<F, UnwrapRefWrapper<Item_>>
    auto inspect(F&& f) &&; // defined in impl/Inspect.hpp

    /// Returns an iterator that only yields items for which the given predicate returns true.
    template <typename F>
    auto filter(F&& f) &&; // defined in impl/Filter.hpp

    /// Similar to filter, but using `std::optional`: the provided function takes an item and
    /// must return an `std::optional<T>`. If the function returns `std::nullopt`, the item is skipped.
    template <typename F> requires std::invocable<F, UnwrapRefWrapper<Item_>>
    auto filterMap(F&& f) &&; // defined in impl/FilterMap.hpp

    /// Returns a concatenation of this iterator and `it2`.
    /// It will yield elements of this iterator until they run out, before switching to `it2`.
    template <typename It2>
    auto chain(It2&& it2) &&; // defined in impl/Chain.hpp

    /// Limits how many items can be yielded by this iterator.
    /// The iterator will not yield any more items after `n` items have been yielded.
    auto take(size_t n) &&; // defined in impl/Take.hpp

    /// Returns an iterator that merges the elements yielded by this iterator and the provided iterator `it2`.
    /// The resulting iterator will yield `std::pair<T, Y>` where `T` is the item type of this iterator and `Y` is the item type of `it2`.
    /// Stops yielding items once any of the two iterator runs out.
    template <typename It2>
    auto zip(It2&& it2) &&; // defined in impl/Zip.hpp

    /// Returns an iterator that skips yielding the first `n` items of this iterator.
    auto skip(size_t n) &&; // defined in impl/Skip.hpp

    /// Returns an iterator that yields the items of this iterator in a cycle, repeating indefinitely.
    auto cycle() &&; // defined in impl/Cycle.hpp

    /// Returns an iterator that groups items of this iterator into statically sized chunks of size `N`.
    /// For example, if this iterator has 10 items and `arrayChunks<3>()` is called,
    /// the resulting iterator will yield `std::array<T, 3>` three times before being exhausted.
    ///
    /// If the amount of items in this iterator is not divisible by `N`, the last few items will be dropped.
    template <size_t N>
    auto arrayChunks() &&; // defined in impl/ArrayChunks.hpp

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
    auto flatten() &&; // defined in impl/Flatten.hpp

    /// Shorthand for `map(f).flatten()`, this allows you to map each item to a container or iterator,
    /// and have the resulting iterator produce the underlying items of those containers/iterators.
    template <typename F> requires std::invocable<F, UnwrapRefWrapper<Item_>>
    auto flatMap(F&& f) && {
        return std::move(*this).map(std::forward<F>(f)).flatten();
    }

    /// Returns whether all items in this iterator match the given predicate, or `true` if there are no items.
    template <typename F> requires std::invocable<F, UnwrapRefWrapper<Item_>>
    bool all(F&& f) && {
        while (auto item = this->derived().next()) {
            if (!std::invoke(f, unwrapRefWrapper(*item))) {
                return false;
            }
        }

        return true;
    }

    /// Returns whether any item in this iterator matches the given predicate, or `false` if there are no items.
    template <typename F> requires std::invocable<F, UnwrapRefWrapper<Item_>>
    bool any(F&& f) && {
        while (auto item = this->derived().next()) {
            if (std::invoke(f, unwrapRefWrapper(*item))) {
                return true;
            }
        }

        return false;
    }

    std::optional<Item> last() && {
        std::optional<Item> lastItem;
        while (auto item = this->derived().next()) {
            lastItem = std::move(*item);
        }
        return lastItem;
    }

    /// Returns the first item that matches the given predicate, or `std::nullopt` if no item matches.
    template <typename F> requires std::invocable<F, UnwrapRefWrapper<Item_>>
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
    /// The collection must implement the `Collector<C>` trait by specializing it.
    /// When collecting an iterator of pairs, the trait is `Collector2<C>`.
    /// Iterators yielding tuples of 3 or more elements are not supported.
    /// Default specializations exist in <asp/iter/CollectorsImpl.hpp>.
    template <typename Container = std::vector<ExtractRefWrapper<Item>>>
    auto collect() && {
        using OutItem = ExtractRefWrapper<Item>;

        std::optional<size_t> sizeHint;

        if constexpr (ProvidesSizeHint<Concrete>) {
            sizeHint = this->derived().sizeHint();
        }

        if constexpr (IsPair<OutItem> && IsCollector2<Container>) {
            // iterator of pairs and the collection supports Collector2 trait, we can use that
            return this->collectPairsInto<Container>(sizeHint);
        } else if constexpr (IsCollector<Container>) {
            return this->collectInto<Container>(sizeHint);
        } else {
            static_assert(
                !std::is_same_v<Container, Container>, "\n\n"
                "Container must implement the `Collector` or `Collector2` trait to use `collect()`.\n"
                "If this is a standard library container, you likely haven't included <asp/iter/Collectors.hpp> which may resolve this problem.\n"
                "Otherwise, you may need to specialize the `Collector` trait for your container type.\n\n"
            );
        }
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
        return std::move(*this).map([n = size_t{0}]<typename T>(T&& item) mutable {
            return std::pair<size_t, UnwrapRefWrapper<Item>>{n++, std::forward<T>(item)};
        });
    }

    /// Folds every element into an accumulator by applying an operation, and returns the final result.
    /// A mutable reference to the `init` argument will be passed as the first argument to the operation, together with the item.
    template <typename B, typename F> requires std::invocable<F, B&, UnwrapRefWrapper<Item_>>
    B fold(B init, F&& f) && {
        while (auto item = this->derived().next()) {
            std::invoke(f, init, unwrapRefWrapper(*item));
        }
        return init;
    }

    /// Shorthand for `map()` with a `static_cast` inside, casts each item to the given type `Y`.
    template <typename Y>
    auto mapCast() && {
        return std::move(*this).map([]<typename T>(T&& item) mutable {
            return static_cast<Y>(std::forward<T>(item));
        });
    }

    /// Consumes the iterator and invokes the given function on every item.
    template <typename F> requires std::invocable<F, UnwrapRefWrapper<Item_>>
    void forEach(F&& f) && {
        while (auto item = this->derived().next()) {
            std::invoke(f, unwrapRefWrapper(*item));
        }
    }

    /// Consumes the iterator without returning anything, processing all intermediate operations.
    void terminate() && {
        while (this->derived().next()) {}
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
    template <typename O = Item>
    O sum() && requires(Summable<O, Item>) {
        O total{};
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

        decltype(auto) operator*() {
            ASP_ASSERT(m_item, "Dereferencing end iterator");
            return unwrapRefWrapper(*m_item);
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

    template <typename C>
    auto collectPairsInto(std::optional<size_t> sizeHint) requires(IsCollector2<C>) {
        Collector2<C> collector{sizeHint};
        while (auto item = this->derived().next()) {
            decltype(auto) pair = unwrapRefWrapper(std::move(*item));

            // there pair can be 3 things:
            // pair ref, aka pair<...>&
            // pair with refs, aka pair<X, Y&> or pair<X&, Y&>
            // pair of owned values
            // we want to move the values only when the pair isn't a reference and the value isn't either
            using First = decltype(pair.first);
            using Second = decltype(pair.second);
            static constexpr bool moveFirst = !std::is_reference_v<decltype(pair)> && !std::is_reference_v<First>;
            static constexpr bool moveSecond = !std::is_reference_v<decltype(pair)> && !std::is_reference_v<Second>;

            if constexpr (moveFirst && moveSecond) {
                collector.push(std::move(pair.first), std::move(pair.second));
            } else if constexpr (moveFirst) {
                collector.push(std::move(pair.first), pair.second);
            } else if constexpr (moveSecond) {
                collector.push(pair.first, std::move(pair.second));
            } else {
                collector.push(pair.first, pair.second);
            }
        }
        return collector.finish();
    }

    template <typename C>
    auto collectInto(std::optional<size_t> sizeHint) requires(IsCollector<C>) {
        Collector<C> collector{sizeHint};

        while (auto item = this->derived().next()) {
            collector.push(unwrapRefWrapper(std::move(*item)));
        }

        return collector.finish();
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

template <typename It>
static constexpr bool CxxCanCalculateDistance = std::forward_iterator<It>;

template <IsCxxIterator It>
class ASP_ITER_NODISCARD CxxIter : public Iter<CxxIter<It>, CxxIterUnderlying<It>> {
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

    size_t sizeHint() const requires(CxxCanCalculateDistance<It>) {
        return std::distance(m_current, m_end);
    }

private:
    It m_current, m_end;
};

template <IsCxxIterator It>
class ASP_ITER_NODISCARD CxxIterConsume : public Iter<CxxIterConsume<It>, std::decay_t<decltype(*std::declval<It>())>> {
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

    size_t sizeHint() const requires(CxxCanCalculateDistance<It>) {
        return std::distance(m_current, m_end);
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
class ASP_ITER_NODISCARD OnceIter : public Iter<OnceIter<T>, T> {
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

template <typename T, bool Forever = false>
class ASP_ITER_NODISCARD RepeatIter : public Iter<RepeatIter<T, Forever>, T> {
public:
    using Item = T;

    template <typename X>
    RepeatIter(X&& value, size_t times) : m_value(std::forward<X>(value)), m_times(times) {}

    template <typename X>
    RepeatIter(X&& value) requires (Forever) : m_value(std::forward<X>(value)), m_times(0) {}

    std::optional<T> next() {
        if constexpr (!Forever) {
            if (m_times == 0) {
                return std::nullopt;
            }
            --m_times;
        }

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
    return RepeatIter<T, true>(std::move(value));
}

}
