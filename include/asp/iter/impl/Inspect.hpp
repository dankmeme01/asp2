#pragma once
#include "../Iter.hpp"

namespace asp::iter {

template <typename It, typename F>
class ASP_ITER_NODISCARD Inspect : public Iter<Inspect<It, F>, typename It::Item> {
public:
    using Item = typename It::Item;

    Inspect(It iter, F func) : m_iter(std::move(iter)), m_func(std::move(func)) {}

    std::optional<Item> next() {
        if (auto item = m_iter.next()) {
            std::invoke(m_func, unwrapRefWrapperRef(*item));
            return item;
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
template <typename F> requires std::invocable<F, UnwrapRefWrapper<Item_>>
auto Iter<Concrete, Item_>::inspect(F&& f) && {
    return Inspect<Concrete, std::decay_t<F>>(
        std::move(this->derived()),
        std::forward<F>(f)
    );
}

}