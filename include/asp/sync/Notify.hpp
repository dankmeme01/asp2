#pragma once

#include <asp/detail/config.hpp>
#include <asp/time/Duration.hpp>
#include <functional>
#include <cstddef>

#ifdef ASP_IS_WIN
struct CRITICAL_SECTION;
struct CONDITION_VARIABLE;
constexpr size_t ASP_NOTIFY_INNER_SIZE = 40 + 8; // size of crit section and condvar in that order
#else
# include <pthread.h>
constexpr size_t ASP_NOTIFY_INNER_SIZE = sizeof(pthread_mutex_t) + sizeof(pthread_cond_t);
#endif

namespace asp {

/// Thread-safe notification mechanism for signaling between threads.
class Notify {
public:
    Notify();
    ~Notify();

    Notify(const Notify&) = delete;
    Notify& operator=(const Notify&) = delete;
    Notify(Notify&&) = delete;
    Notify& operator=(Notify&&) = delete;

    /// Waits for a notification
    void wait();

    /// Waits for a notification with a timeout.
    /// Returns true if notified, false if timeout occurred.
    bool wait(const time::Duration& timeout);

    /// Waits for a predicate to become true, or until a timeout occurs.
    /// Returns true if the predicate is true, false if timeout occurred.
    /// Specify the timeout as 0 to wait indefinitely.
    bool wait(const time::Duration& timeout, const std::function<bool()>& predicate);

    /// Notifies one waiting thread
    void notifyOne();

    /// Notifies all waiting threads
    void notifyAll();

private:
#ifdef ASP_IS_WIN
    alignas(void*) char _storage[ASP_NOTIFY_INNER_SIZE];
    CRITICAL_SECTION* _crit();
    CONDITION_VARIABLE* _cond();
#else
    alignas(std::max_align_t) char _storage[ASP_NOTIFY_INNER_SIZE];
    pthread_mutex_t* _mutex();
    pthread_cond_t* _cond();
#endif

};

}