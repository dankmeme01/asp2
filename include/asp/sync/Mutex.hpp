#pragma once
#include "../detail/config.hpp"
#include <utility>
#include <mutex>

namespace asp {

template <typename T>
class Channel;

template <typename T, bool Recursive>
class Mutex;

template <typename T = void, bool Recursive = false>
class [[nodiscard("A mutex guard must be stored in a variable to be effective")]] MutexGuardBase {
public:
    MutexGuardBase(const MutexGuardBase&) = delete;
    MutexGuardBase& operator=(const MutexGuardBase&) = delete;

    MutexGuardBase(Mutex<T, Recursive>& mutex) : mtx(&mutex) {
        mtx->m_mtx.lock();
        locked = true;
    }

    MutexGuardBase(MutexGuardBase&& other) noexcept {
        *this = std::move(other);
    }

    MutexGuardBase& operator=(MutexGuardBase&& other) noexcept {
        if (this != &other) {
            this->mtx = other.mtx;
            this->locked = other.locked;

            other.mtx = nullptr;
            other.locked = false;
        }
        return *this;
    }

    ~MutexGuardBase() {
        this->unlock();
    }

    // Unlocks the mutex. Any access to this `Guard` afterwards invokes undefined behavior,
    // unless it is relocked again with `.relock()`.
    void unlock() {
        if (!locked) return;

        mtx->m_mtx.unlock();
        locked = false;
    }

    void relock() {
        if (locked) return;

        mtx->m_mtx.lock();
        locked = true;
    }

protected:
    Mutex<T, Recursive>* mtx;
    bool locked = false;
};

template <typename T = void, bool Recursive = false>
struct MutexGuard : public MutexGuardBase<T, Recursive> {
    T& operator*() {
        return this->mtx->m_data;
    }

    const T& operator*() const {
        return this->mtx->m_data;
    }

    T* operator->() {
        return &this->mtx->m_data;
    }

    const T* operator->() const {
        return &this->mtx->m_data;
    }

    MutexGuard& operator=(const T& other) {
        this->mtx->m_data = other;
        return *this;
    }

    MutexGuard& operator=(T&& other) {
        this->mtx->m_data = std::move(other);
        return *this;
    }
};

// Specializations for void and non void
template <bool Recursive>
class MutexGuard<void, Recursive> : public MutexGuardBase<void, Recursive> {
    // nothing!
};

template <typename T = void, bool Recursive = false>
class MutexBase {
public:
    using MutexType = std::conditional_t<
        Recursive,
        std::recursive_mutex,
        std::mutex>;

    MutexBase() = default;

    MutexBase(const MutexBase&) = delete;
    MutexBase(MutexBase&&) = delete;
    MutexBase& operator=(const MutexBase&) = delete;
    MutexBase& operator=(MutexBase&&) = delete;

protected:
    template <typename U>
    friend class Channel;
    template <typename U, bool R>
    friend class MutexGuardBase;

    mutable MutexType m_mtx;
};

template <typename T = void, bool Recursive = false>
class Mutex : public MutexBase<T, Recursive> {
public:
    using Guard = MutexGuard<T, Recursive>;

    template <typename... Args>
    Mutex(Args&&... args) : m_data(std::forward<Args>(args)...) {}

    Guard lock() const {
        return Guard(const_cast<Mutex<T, Recursive>&>(*this));
    }

private:
    friend class MutexGuard<T, Recursive>;

    T m_data;
};

template <bool Recursive>
class Mutex<void, Recursive> : public MutexBase<void, Recursive> {
public:
    using Guard = MutexGuard<void, Recursive>;

    Mutex() = default;

    Guard lock() const {
        return Guard(const_cast<Mutex<void, Recursive>&>(*this));
    }

private:
    friend class MutexGuard<void, Recursive>;
};

}
