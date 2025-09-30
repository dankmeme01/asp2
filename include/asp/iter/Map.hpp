#pragma once
#include <utility>
#include <optional>
#include <functional>
#include "Iter.hpp"

namespace asp::iter {

template <typename It, typename F>
class Map : public Iter<Map<It, F>, std::invoke_result_t<F, typename It::Item>> {
public:
    using Item = std::invoke_result_t<F, typename It::Item>;

    Map(It iter, F func) : m_iter(std::move(iter)), m_func(std::move(func)) {}

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
auto Iter<Concrete, Item_>::map(F&& f) && {
    return Map<Concrete, std::decay_t<F>>(
        std::move(this->derived()),
        std::forward<F>(f)
    );
}

}