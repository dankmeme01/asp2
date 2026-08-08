#pragma once

#include <asp/detail/config.hpp>
#include <asp/detail/Function.hpp>
#include <asp/time/Duration.hpp>

#ifndef ASP_IS_WIN
# include <pthread.h>
#else

struct _RTL_CRITICAL_SECTION;
typedef struct _RTL_CRITICAL_SECTION CRITICAL_SECTION;
struct _RTL_CONDITION_VARIABLE;
typedef struct _RTL_CONDITION_VARIABLE CONDITION_VARIABLE;

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
    bool wait(const time::Duration& timeout, asp::FunctionRef<bool()> predicate);

    /// Notifies one waiting thread
    void notifyOne();

    /// Notifies all waiting threads
    void notifyAll();

private:
#ifdef ASP_IS_WIN
    alignas(8) char _critStorage[40];
    alignas(8) char _condStorage[8];

    CRITICAL_SECTION* _crit();
    CONDITION_VARIABLE* _cond();
#else
    pthread_mutex_t _mutexStorage;
    pthread_cond_t _condStorage;

    pthread_mutex_t* _mutex();
    pthread_cond_t* _cond();
#endif

};

}