#pragma once
#include "../Iter.hpp"

namespace asp::iter {

template <typename T>
struct ExtractOptional;

template <typename T>
struct ExtractOptional<std::optional<T>> {
    using type = T;
};

template <typename It, typename F>
using FilterMapInvokeResult = std::invoke_result_t<F, UnwrapRefWrapper<typename It::Item>>;

template <typename It, typename F>
using FilterMapItem = typename ExtractOptional<FilterMapInvokeResult<It, F>>::type;

template <typename It, typename F>
class ASP_ITER_NODISCARD FilterMap : public Iter<FilterMap<It, F>, FilterMapItem<It, F>> {
public:
    using Item = FilterMapItem<It, F>;
    static_assert(std::is_same_v<FilterMapInvokeResult<It, F>, std::optional<Item>>, "FilterMap function must return std::optional");

    FilterMap(It iter, F func) : m_iter(std::move(iter)), m_func(std::move(func)) {}

    std::optional<Item> next() {
        while (auto item = m_iter.next()) {
            if (auto ret = std::invoke(m_func, unwrapRefWrapper(std::move(*item)))) {
                return ret;
            }
        }

        return std::nullopt;
    }

private:
    It m_iter;
    F m_func;
};

template <typename Concrete, typename Item_>
template <typename F> requires std::invocable<F, UnwrapRefWrapper<Item_>>
auto Iter<Concrete, Item_>::filterMap(F&& f) && {
    return FilterMap<Concrete, std::decay_t<F>>(
        std::move(this->derived()),
        std::forward<F>(f)
    );
}

}