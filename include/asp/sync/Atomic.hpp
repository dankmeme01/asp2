#pragma once

#include "../detail/config.hpp"
#include <atomic>

namespace asp {

// Simple wrapper around atomics with the default memory order set to relaxed instead of seqcst, plus a copy constructor
template <typename T, typename Inner = std::atomic<T>>
class Atomic {
public:
    // idk what im doing send help
    template <typename U = T, std::enable_if_t<!std::is_void_v<U>, U> = 0>
    Atomic(T initial = {}) : value(initial) {}

    template <typename U = T, std::enable_if_t<!std::is_void_v<U>, U> = 0>
    T load(std::memory_order order = std::memory_order::relaxed) const {
        return value.load(order);
    }

    template <typename U = T, std::enable_if_t<!std::is_void_v<U>, U> = 0>
    void store(T val, std::memory_order order = std::memory_order::relaxed) {
        value.store(val, order);
    }

    template <typename U = T, std::enable_if_t<!std::is_void_v<U>, U> = 0>
    operator T() const {
        return load();
    }

    template <typename U = T, std::enable_if_t<!std::is_void_v<U>, U> = 0>
    Atomic<T, Inner>& operator=(T val) {
        this->store(val);
        return *this;
    }

    // enable copying, it is disabled by default in std::atomic
    template <typename U = T, std::enable_if_t<!std::is_void_v<U>, U> = 0>
    Atomic(const Atomic<T, Inner>& other) {
        this->store(other.load());
    }

    template <typename U = T, std::enable_if_t<!std::is_void_v<U>, U> = 0>
    Atomic<T, Inner>& operator=(const Atomic<T, Inner>& other) {
        if (this != &other) {
            this->store(other.load());
        }

        return *this;
    }

    // enable moving
    template <typename U = T, std::enable_if_t<!std::is_void_v<U>, U> = 0>
    Atomic(Atomic<T, Inner>&& other) {
        this->store(other.load());
    }

    template <typename U = T, std::enable_if_t<!std::is_void_v<U>, U> = 0>
    Atomic& operator=(Atomic<T, Inner>&& other) {
        if (this != &other) {
            this->store(other.load());
        }

        return *this;
    }

private:
    Inner value;
};

using AtomicFlag = Atomic<void, std::atomic_flag>;
using AtomicBool = Atomic<bool>;
using AtomicChar = Atomic<char>;
using AtomicI8 = Atomic<int8_t>;
using AtomicU8 = Atomic<uint8_t>;
using AtomicI16 = Atomic<int16_t>;
using AtomicU16 = Atomic<uint16_t>;
using AtomicInt = Atomic<int>;
using AtomicI32 = Atomic<int32_t>;
using AtomicU32 = Atomic<uint32_t>;
using AtomicI64 = Atomic<int64_t>;
using AtomicU64 = Atomic<uint64_t>;
using AtomicF32 = Atomic<float, std::atomic<uint32_t>>;
using AtomicSizeT = Atomic<size_t>;



template <>
class Atomic<void, std::atomic_flag> {
public:
    Atomic() {}

    bool test(std::memory_order order = std::memory_order::relaxed) const {
        return value.test(order);
    }

    void clear(std::memory_order order = std::memory_order::relaxed) {
        value.clear(order);
    }

    void set(std::memory_order order = std::memory_order::relaxed) {
        this->testAndSet(order);
    }

    bool testAndSet(std::memory_order order = std::memory_order::relaxed) {
        return value.test_and_set(order);
    }

    operator bool() const {
        return this->test();
    }

    Atomic<void, std::atomic_flag>& operator=(bool val) {
        val ? this->set() : this->clear();
        return *this;
    }

    // copying
    Atomic(const Atomic<void, std::atomic_flag>& other) {
        other.test() ? this->set() : this->clear();
    }

    Atomic<void, std::atomic_flag>& operator=(const Atomic<void, std::atomic_flag>& other) {
        if (this != &other) {
            other.test() ? this->set() : this->clear();
        }

        return *this;
    }

    // moving
    Atomic(Atomic<void, std::atomic_flag>&& other) {
        other.test() ? this->set() : this->clear();
    }

    Atomic<void, std::atomic_flag>& operator=(Atomic<void, std::atomic_flag>&& other) {
        if (this != &other) {
            other.test() ? this->set() : this->clear();
        }

        return *this;
    }

private:
    std::atomic_flag value;
};

float _u32tof32(uint32_t num);
uint32_t _f32tou32(float num);

template <>
class Atomic<float, std::atomic<uint32_t>> {
public:
    Atomic(float initial = {}) : value(_f32tou32(initial)) {}

    float load(std::memory_order order = std::memory_order::relaxed) const {
        return _u32tof32(value.load(order));
    }

    void store(float val, std::memory_order order = std::memory_order::relaxed) {
        value.store(_f32tou32(val), order);
    }

    operator float() const {
        return this->load();
    }

    Atomic<float, std::atomic<uint32_t>>& operator=(float val) {
        this->store(val);
        return *this;
    }

    // enable copying, it is disabled by default in std::atomic
    Atomic(Atomic<float, std::atomic<uint32_t>>& other) {
        this->store(other.load());
    }

    Atomic<float, std::atomic<uint32_t>>& operator=(const Atomic<float, std::atomic<uint32_t>>& other) {
        if (this != &other) {
            this->store(other.load());
        }

        return *this;
    }

    // moving
    Atomic(Atomic<float, std::atomic<uint32_t>>&& other) {
        this->store(other.load());
    }

    Atomic<float, std::atomic<uint32_t>>& operator=(Atomic<float, std::atomic<uint32_t>>&& other) {
        if (this != &other) {
            this->store(other.load());
        }

        return *this;
    }
private:
    std::atomic<uint32_t> value;
};

}