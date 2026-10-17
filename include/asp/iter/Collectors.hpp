#pragma once
#include <Geode/Result.hpp>
#include "Iter.hpp"
#include <deque>
#include <queue>
#include <vector>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>

namespace asp::iter {

template <typename T>
struct Collector<std::vector<T>> {
    Collector(std::optional<size_t> sizeHint) {
        if (sizeHint) {
            m_container.reserve(*sizeHint);
        }
    }

    template <typename Y>
    void push(Y&& item) {
        m_container.emplace_back(std::forward<Y>(item));
    }

    auto finish() {
        return std::move(m_container);
    }

private:
    std::vector<T> m_container;
};

template <typename T>
struct Collector<std::deque<T>> {
    Collector(std::optional<size_t> sizeHint) {}

    template <typename Y>
    void push(Y&& item) {
        m_container.emplace_back(std::forward<Y>(item));
    }

    auto finish() {
        return std::move(m_container);
    }

private:
    std::deque<T> m_container;
};

template <typename T>
struct Collector<std::queue<T>> {
    Collector(std::optional<size_t> sizeHint) {}

    template <typename Y>
    void push(Y&& item) {
        m_container.push(std::forward<Y>(item));
    }

    auto finish() {
        return std::move(m_container);
    }

private:
    std::queue<T> m_container;
};

template <typename T>
struct Collector<std::unordered_set<T>> {
    Collector(std::optional<size_t> sizeHint) {
        if (sizeHint) {
            m_container.reserve(*sizeHint);
        }
    }

    template <typename Y>
    void push(Y&& item) {
        m_container.insert(std::forward<Y>(item));
    }

    auto finish() {
        return std::move(m_container);
    }

private:
    std::unordered_set<T> m_container;
};

template <typename T>
struct Collector<std::set<T>> {
    Collector(std::optional<size_t> sizeHint) {}

    template <typename Y>
    void push(Y&& item) {
        m_container.insert(std::forward<Y>(item));
    }

    auto finish() {
        return std::move(m_container);
    }

private:
    std::set<T> m_container;
};

template <typename T, typename Y>
struct Collector2<std::unordered_map<T, Y>> {
    Collector2(std::optional<size_t> sizeHint) {}

    template <typename K, typename V>
    void push(K&& item1, V&& item2) {
        m_container.emplace(std::forward<K>(item1), std::forward<V>(item2));
    }

    auto finish() {
        return std::move(m_container);
    }

private:
    std::unordered_map<T, Y> m_container;
};

template <typename T, typename Y>
struct Collector2<std::map<T, Y>> {
    Collector2(std::optional<size_t> sizeHint) {}

    template <typename K, typename V>
    void push(K&& item1, V&& item2) {
        m_container.emplace(std::forward<K>(item1), std::forward<V>(item2));
    }

    auto finish() {
        return std::move(m_container);
    }

private:
    std::map<T, Y> m_container;
};

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
