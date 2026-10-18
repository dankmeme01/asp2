#pragma once
#include <asp/iter/Iter.hpp>
#include <Geode/Result.hpp>

namespace asp::iter {

// hacky result collector
template <typename T, typename E>
struct Collector<geode::Result<std::vector<T>, E>> {
    Collector(std::optional<size_t> sizeHint) {
        if (sizeHint) {
            m_container.reserve(*sizeHint);
        }
    }

    template <typename R> requires geode::IsResult<std::decay_t<R>>
    void push(R&& item) {
        if (item.isOk()) {
            if (!m_error) {
                m_container.emplace_back(std::forward<R>(item).unwrap());
            }
        } else {
            if (!m_error) {
                m_error = std::forward<R>(item).unwrapErr();
            }
        }
    }

    auto finish() -> geode::Result<std::vector<T>, E> {
        if (m_error) {
            return geode::Err(std::move(*m_error));
        } else {
            return geode::Ok(std::move(m_container));
        }
    }

private:
    std::vector<T> m_container;
    std::optional<E> m_error;
};

}

