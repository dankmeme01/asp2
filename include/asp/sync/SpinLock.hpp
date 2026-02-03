#pragma once
#include "../detail/config.hpp"
#include <utility>
#include <stdint.h>

#ifdef ASP_IS_X86
# include <immintrin.h>
#endif

#ifndef __clang__
# include <intrin.h>
#endif

namespace asp {

#ifdef __clang__

void ASP_FORCE_INLINE acquireAtomicLock(volatile uint8_t* ptr) {
    uint8_t expected = 0;

    while (!__atomic_compare_exchange_n(ptr, &expected, 1, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)) {
        expected = 0;

#ifdef ASP_IS_X86
        _mm_pause();
#else
        __asm__ __volatile__("yield");
#endif
    }
}

void ASP_FORCE_INLINE releaseAtomicLock(volatile uint8_t* obj) {
    __atomic_store_n(obj, 0, __ATOMIC_RELEASE);
}

#else

void ASP_FORCE_INLINE acquireAtomicLock(volatile uint8_t* ptr) {
    while (true) {
        if (_InterlockedCompareExchange8((volatile char*)ptr, 1, 0) == 0) {
            return;
        }

        // spin
        _mm_pause();
    }
}

void ASP_FORCE_INLINE releaseAtomicLock(volatile uint8_t* obj) {
    _ReadWriteBarrier();
    *obj = 0;
}

#endif

template <typename T>
class Channel;

template <typename Inner = void>
class SpinLock {
public:
    SpinLock() : data(), mtx() {}
    SpinLock(Inner&& obj) : data(std::move(obj)), mtx() {}
    SpinLock(const Inner& obj) : data(obj), mtx() {}

    SpinLock(const SpinLock&) = delete;
    SpinLock(SpinLock&&) = delete;

    SpinLock& operator=(const SpinLock&) = delete;
    SpinLock& operator=(SpinLock&&) = delete;

    class [[nodiscard("A mutex guard must be stored in a variable to be effective")]] Guard {
    public:
        Guard(const Guard&) = delete;
        Guard& operator=(const Guard&) = delete;

        Guard(const SpinLock& mtx) : mtx(mtx) {
            mtx._lock();
        }

        inline ~Guard() {
            this->unlock();
        }

        // Unlocks the mutex. Any access to this `Guard` afterwards invokes undefined behavior,
        // unless it is relocked again with `.relock()`.
        inline void unlock() {
            if (!alreadyUnlocked) {
                mtx._unlock();

                alreadyUnlocked = true;
            }
        }

        // Relocks the mutex after being unlocked with `unlock()`.
        // If the mutex was already locked, this does nothing.
        inline void relock() {
            if (alreadyUnlocked) {
                mtx._lock();
                alreadyUnlocked = false;
            }
        }

        Inner& operator*() {
            return mtx.data;
        }

        const Inner& operator*() const {
            return mtx.data;
        }

        Inner* operator->() {
            return &mtx.data;
        }

        const Inner* operator->() const {
            return &mtx.data;
        }

        Guard& operator=(const Inner& rhs) {
            mtx.data = rhs;
            return *this;
        }

        Guard& operator=(Inner&& rhs) {
            mtx.data = std::move(rhs);
            return *this;
        }
    private:
        const SpinLock& mtx;
        bool alreadyUnlocked = false;
    };

    Guard lock() const {
        return Guard(*this);
    }

private:
    friend class Guard;

    template <typename T>
    friend class Channel;

    mutable Inner data;
    mutable uint8_t mtx = 0;

    void _lock() const {
        acquireAtomicLock(&mtx);
    }

    void _unlock() const {
        releaseAtomicLock(&mtx);
    }
};

/* Specialization for SpinLock<void> */
template <>
class SpinLock<void> {
public:
    SpinLock() : mtx() {}

    SpinLock(const SpinLock&) = delete;
    SpinLock(SpinLock&&) = delete;

    SpinLock& operator=(const SpinLock&) = delete;
    SpinLock& operator=(SpinLock&&) = delete;

    class Guard {
    public:
        inline Guard(const SpinLock& mtx) : mtx(mtx) {
            mtx._lock();
        }

        inline ~Guard() {
            this->unlock();
        }

        // Unlocks the mutex. Any access to this `Guard` afterwards invokes undefined behavior,
        // unless it is relocked again with `.relock()`.
        inline void unlock() {
            if (!alreadyUnlocked) {
                mtx._unlock();
                alreadyUnlocked = true;
            }
        }

        // Relocks the mutex after being unlocked with `unlock()`.
        // If the mutex was already locked, this does nothing.
        inline void relock() {
            if (alreadyUnlocked) {
                mtx._lock();
                alreadyUnlocked = false;
            }
        }

        Guard(const Guard&) = delete;
        Guard& operator=(const Guard&) = delete;
    private:
        const SpinLock& mtx;
        bool alreadyUnlocked = false;
    };

    inline Guard lock() const {
        return Guard(*this);
    }
private:
    mutable uint8_t mtx = 0;

    void _lock() const {
        acquireAtomicLock(&mtx);
    }

    void _unlock() const {
        releaseAtomicLock(&mtx);
    }
};

template <typename T>
using SpinLockGuard = typename SpinLock<T>::Guard;

}