#pragma once
#include "../Iter.hpp"

namespace asp::iter {

template <typename It, typename F>
using MapItem = WrapRefWrapper<std::invoke_result_t<F, UnwrapRefWrapper<typename It::Item>>>;

template <typename It, typename F>
class ASP_ITER_NODISCARD Map : public Iter<Map<It, F>, MapItem<It, F>> {
public:
    using Item = MapItem<It, F>;

    Map(It iter, F func) : m_iter(std::move(iter)), m_func(std::move(func)) {}

    std::optional<Item> next() {
        if (auto item = m_iter.next()) {
            return wrapRefWrapper(std::invoke(m_func, unwrapRefWrapper(std::move(*item))));
        }

        return std::nullopt;
    }

    size_t sizeHint() const requires(ProvidesSizeHint<It>) {
        return m_iter.sizeHint();
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