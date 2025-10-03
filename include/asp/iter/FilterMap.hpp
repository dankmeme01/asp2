#pragma once
#include <utility>
#include <optional>
#include <functional>
#include "Iter.hpp"

namespace asp::iter {

template <typename T>
struct ExtractOptional;

template <typename T>
struct ExtractOptional<std::optional<T>> {
    using type = T;
};

template <typename It, typename F>
class FilterMap : public Iter<FilterMap<It, F>, typename ExtractOptional<std::invoke_result_t<F, typename It::Item>>::type> {
public:
    using Item = typename ExtractOptional<std::invoke_result_t<F, typename It::Item>>::type;
    static_assert(std::is_same_v<std::invoke_result_t<F, typename It::Item>, std::optional<Item>>, "FilterMap function must return std::optional");

    FilterMap(It iter, F func) : m_iter(std::move(iter)), m_func(std::move(func)) {}

    std::optional<Item> next() {
        if (auto item = m_iter.next()) {
            return std::invoke(m_func, std::move(*item));
        }

        return std::nullopt;
    }

private:
    It m_iter;
    F m_func;
};

template <typename Concrete, typename Item_>
template <typename F>
auto Iter<Concrete, Item_>::filterMap(F&& f) && {
    return FilterMap<Concrete, std::decay_t<F>>(
        std::move(this->derived()),
        std::forward<F>(f)
    );
}

}