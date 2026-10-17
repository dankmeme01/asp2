#pragma once
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


}
