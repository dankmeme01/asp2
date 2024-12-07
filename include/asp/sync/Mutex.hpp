#pragma once
#include "../detail/config.hpp"
#include <utility>
#include <boost/thread/mutex.hpp>

#ifdef ASP_DEBUG
#include "DeadlockGuard.hpp"
#endif

namespace asp {

template <typename T>
class Channel;

template <typename Inner = void>
class Mutex {
public:
    Mutex() : data(), mtx() {}
    Mutex(Inner&& obj) : data(std::move(obj)), mtx() {}
    Mutex(const Inner& obj) : data(obj), mtx() {}

    Mutex(const Mutex&) = delete;
    Mutex(Mutex&&) = delete;

    Mutex& operator=(const Mutex&) = delete;
    Mutex& operator=(Mutex&&) = delete;

    class Guard {
    public:
        Guard(const Guard&) = delete;
        Guard& operator=(const Guard&) = delete;

        Guard(const Mutex& mtx) : mtx(mtx) {
#ifdef ASP_DEBUG
            mtx.dlGuard.lockAttempt();
            mtx.mtx.lock();
            mtx.dlGuard.lockSuccess();
#else
            mtx.mtx.lock();
#endif
        }

        ~Guard() {
            this->unlock();
        }

        // Unlocks the mutex. Any access to this `Guard` afterwards invokes undefined behavior.
        void unlock() {
            if (!alreadyUnlocked) {
#ifdef ASP_DEBUG
                mtx.dlGuard.unlock();
#endif
                mtx.mtx.unlock();

                alreadyUnlocked = true;
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
        const Mutex& mtx;
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
    mutable boost::mutex mtx;
#ifdef ASP_DEBUG
    mutable DeadlockGuard dlGuard;
#endif
};

/* Specialization for Mutex<void> */
template <>
class Mutex<void> {
public:
    Mutex() : mtx() {}

    Mutex(const Mutex&) = delete;
    Mutex(Mutex&&) = delete;

    Mutex& operator=(const Mutex&) = delete;
    Mutex& operator=(Mutex&&) = delete;

    class Guard {
    public:
        Guard(const Mutex& mtx) : mtx(mtx) {
#ifdef ASP_DEBUG
            mtx.dlGuard.lockAttempt();
            mtx.mtx.lock();
            mtx.dlGuard.lockSuccess();
#else
            mtx.mtx.lock();
#endif
        }

        ~Guard() {
            this->unlock();
        }

        // Unlocks the mutex. Any access to this `Guard` afterwards invokes undefined behavior.
        void unlock() {
            if (!alreadyUnlocked) {
#ifdef ASP_DEBUG
                mtx.dlGuard.unlock();
#endif
                mtx.mtx.unlock();

                alreadyUnlocked = true;
            }
        }

        Guard(const Guard&) = delete;
        Guard& operator=(const Guard&) = delete;
    private:
        const Mutex& mtx;
        bool alreadyUnlocked = false;
    };

    Guard lock() const {
        return Guard(*this);
    }
private:
    mutable boost::mutex mtx;
#ifdef ASP_DEBUG
    mutable DeadlockGuard dlGuard;
#endif
};

}