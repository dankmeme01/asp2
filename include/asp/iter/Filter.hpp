#pragma once
#include <utility>
#include <optional>
#include <functional>
#include "Iter.hpp"

namespace asp::iter {

template <typename It, typename F>
class Filter : public Iter<Filter<It, F>, typename It::Item> {
public:
    using Item = typename It::Item;

    Filter(It iter, F func) : m_iter(std::move(iter)), m_func(std::move(func)) {}

    std::optional<Item> next() {
        while (auto item = m_iter.next()) {
            if (std::invoke(m_func, *item)) {
                return item;
            }
        }

        return std::nullopt;
    }

private:
    It m_iter;
    F m_func;
};

template <typename Concrete, typename Item_>
template <typename F>
auto Iter<Concrete, Item_>::filter(F&& f) && {
    return Filter<Concrete, std::decay_t<F>>(
        std::move(this->derived()),
        std::forward<F>(f)
    );
}

}