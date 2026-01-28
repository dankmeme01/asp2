#pragma once
#include <asp/detail/config.hpp>
#include <stddef.h>
#include <utility>
#include <stdexcept>
#include <memory>
#include <new>

// the entirety of this file isn't very safe lol

namespace asp {

namespace detail {

template <typename T>
struct Uninit {
    alignas(T) char data[sizeof(T)];

    T* ptr() {
        return std::launder(reinterpret_cast<T*>(&data));
    }

    T& get() {
        return *ptr();
    }

    template <typename... Args>
    void init(Args&&... args) {
        new (ptr()) T(std::forward<Args>(args)...);
    }

    void destroy() {
        ptr()->~T();
    }
};

inline size_t nextPowerOfTwo(size_t n) {
    if (n == 0) return 1;
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    if (sizeof(size_t) == 8) {
        n |= n >> 32;
    }
    return n + 1;
}

}

template<typename T, size_t N>
class SmallVec {
public:
    SmallVec() = default;

    ~SmallVec() {
        this->clear();
        if (isLarge()) {
            delete[] m_large;
        }
    }

    SmallVec(const SmallVec& other) = delete;
    SmallVec& operator=(const SmallVec& other) = delete;

    SmallVec(SmallVec&& other) noexcept {
        *this = std::move(other);
    }

    SmallVec& operator=(SmallVec&& other) noexcept {
        if (this == &other) {
            return *this;
        }

        this->clear();
        if (this->isLarge()) {
            delete[] m_large;
        }

        if (other.isLarge()) {
            m_capacity = other.m_capacity;
            m_size = other.m_size;
            m_large = other.m_large;
            other.m_large = nullptr;
            other.m_size = 0;
            other.m_capacity = N;
        } else {
            m_capacity = N;
            m_size = other.m_size;

            for (size_t i = 0; i < m_size; i++) {
                m_small[i].init(std::move(other.m_small[i].get()));
                other.m_small[i].destroy();
            }
        }

        return *this;
    }

    size_t size() const noexcept {
        return m_size;
    }

    size_t capacity() const noexcept {
        return m_capacity;
    }

    bool empty() const noexcept {
        return m_size == 0;
    }

    T* data() noexcept {
        return this->isLarge() ? m_large->ptr() : m_small[0].ptr();
    }

    const T* data() const noexcept {
        return this->isLarge() ? m_large->ptr() : m_small[0].ptr();
    }

    T& operator[](size_t index) noexcept {
        return data()[index];
    }

    T& at(size_t index) {
        if (index >= m_size) {
            throw std::out_of_range("SmallVec::at: index out of range");
        }
        return data()[index];
    }

    void reserve(size_t newCap) {
        if (newCap <= m_capacity) {
            return;
        }

        auto newStorage = std::make_unique<detail::Uninit<T>[]>(newCap);
        auto data = idata();

        // move elements
        for (size_t i = 0; i < m_size; i++) {
            newStorage[i].init(std::move(data[i].get()));
            data[i].destroy();
        }

        if (this->isLarge()) {
            delete[] m_large;
        }

        m_large = newStorage.release();
        m_capacity = newCap;
    }

    template <typename... Args>
    void emplace_back(Args&&... args) {
        this->growFor(1);
        new (data() + m_size) T(std::forward<Args>(args)...);
        m_size++;
    }

    void push_back(const T& value) {
        this->emplace_back(value);
    }

    void push_back(T&& value) {
        this->emplace_back(std::move(value));
    }

    T* insert(T* iter, const T& value) {
        auto* ptr = this->shift(iter, 1);
        ptr->init(value);
        return ptr->ptr();
    }

    T* insert(T* iter, T&& value) {
        auto* ptr = this->shift(iter, 1);
        ptr->init(std::move(value));
        return ptr->ptr();
    }

    void insert(T* iter, size_t count, const T& value) {
        auto* ptr = this->shift(iter, static_cast<ptrdiff_t>(count));
        for (size_t i = 0; i < count; i++) {
            ptr[i].init(value);
        }
    }

    void insert(T* iter, T const* first, T const* last) {
        size_t count = std::distance(first, last);
        auto* ptr = this->shift(iter, static_cast<ptrdiff_t>(count));
        for (size_t i = 0; i < count; i++, ++first) {
            ptr[i].init(*first);
        }
    }

    T* erase(T* iter) {
        return this->erase(iter, iter + 1);
    }

    T* erase(T* first, T* last) {
        if (first < data() || last > data() + m_size || first > last) {
            throw std::out_of_range("SmallVec::erase: iterator range out of range");
        }
        size_t count = std::distance(first, last);
        return this->shift(first + count, -static_cast<ptrdiff_t>(count))->ptr();
    }

    void pop_back() {
        if (m_size == 0) {
            throw std::out_of_range("SmallVec::pop_back: empty vector");
        }
        m_size--;
        idata()[m_size].destroy();
    }

    T* begin() noexcept {
        return data();
    }

    T* end() noexcept {
        return data() + m_size;
    }

    const T* begin() const noexcept {
        return data();
    }

    const T* end() const noexcept {
        return data() + m_size;
    }

    T& front() {
        if (m_size == 0) {
            throw std::out_of_range("SmallVec::front: empty vector");
        }
        return data()[0];
    }

    T& back() {
        if (m_size == 0) {
            throw std::out_of_range("SmallVec::back: empty vector");
        }
        return data()[m_size - 1];
    }

    void clear() noexcept {
        for (size_t i = 0; i < m_size; i++) {
            idata()[i].destroy();
        }
        m_size = 0;
    }

private:
    union {
        detail::Uninit<T> m_small[N];
        detail::Uninit<T>* m_large;
    };
    size_t m_size = 0;
    size_t m_capacity = N;

    detail::Uninit<T>* idata() {
        return this->isLarge() ? m_large : &m_small[0];
    }

    /// Shifts elements starting from 'iter' by 'offset' positions (positive or negative)
    /// Returns the new location of 'iter'
    detail::Uninit<T>* shift(T* iter, ptrdiff_t offset) {
        ptrdiff_t index = std::distance(data(), iter);
        ASP_ASSERT(index >= 0 && static_cast<size_t>(index) <= m_size, "SmallVec::shift: iterator out of range");

        if (offset > 0) {
            this->growFor(static_cast<size_t>(offset));
        }

        if (offset > 0) {
            // move elements [index, m_size) to the right by offset
            for (ptrdiff_t i = m_size - 1; i >= index; i--) {
                this->moveOne(i, i + offset);
            }
        } else if (offset < 0) {
            // move elements [index, m_size) to the left by -offset
            for (ptrdiff_t i = index; i < m_size; ++i) {
                this->moveOne(i, i + offset);
            }
        }

        m_size += offset;
        return &idata()[index];
    }

    /// Moves element from index 'from' to index 'to', destroys original element,
    /// and destroys the destination element if it existed.
    void moveOne(size_t from, size_t to) {
        if (from == to) [[unlikely]] return;

        auto data = this->idata();

        if (to < m_size) {
            data[to].destroy();
        }

        data[to].init(std::move(data[from].get()));
        data[from].destroy();
    }

    bool isLarge() const {
        return m_capacity > N;
    }

    void growFor(size_t extra) {
        if (m_size + extra <= m_capacity) {
            return;
        }

        auto newCap = std::max<size_t>(m_capacity, 1);
        while (newCap < m_size + extra) {
            newCap = detail::nextPowerOfTwo(newCap + 1);
        }
        this->reserve(newCap);
    }
};

}