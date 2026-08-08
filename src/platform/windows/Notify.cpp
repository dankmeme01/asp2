#include <asp/sync/Notify.hpp>
#include <asp/time/Instant.hpp>

#ifndef WIN32_LEAN_AND_MEAN
# define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

namespace asp {

CRITICAL_SECTION* Notify::_crit() {
    static_assert(sizeof(_critStorage) == sizeof(CRITICAL_SECTION));
    return (CRITICAL_SECTION*)&_critStorage[0];
}

CONDITION_VARIABLE* Notify::_cond() {
    static_assert(sizeof(_condStorage) == sizeof(CONDITION_VARIABLE));
    return (CONDITION_VARIABLE*)&_condStorage[0];
}

Notify::Notify() {
    InitializeCriticalSection(_crit());
    InitializeConditionVariable(_cond());
}

Notify::~Notify() {
    DeleteCriticalSection(_crit());
    // CONDITION_VARIABLE does not need explicit deletion
}

void Notify::wait() {
    EnterCriticalSection(_crit());
    SleepConditionVariableCS(_cond(), _crit(), INFINITE);
    LeaveCriticalSection(_crit());
}

bool Notify::wait(const time::Duration& timeout) {
    EnterCriticalSection(_crit());
    bool result = SleepConditionVariableCS(_cond(), _crit(), timeout.millis<u64>());
    LeaveCriticalSection(_crit());
    return result;
}

bool Notify::wait(const time::Duration& timeout, asp::FunctionRef<bool()> predicate) {
    EnterCriticalSection(_crit());

    if (timeout.isZero()) {
        // If timeout is zero, we wait indefinitely
        while (!predicate()) {
            SleepConditionVariableCS(_cond(), _crit(), INFINITE);
        }

        LeaveCriticalSection(_crit());
        return true;
    }

    auto begin = time::Instant::now();

    while (!predicate()) {
        auto leftTime = timeout - begin.elapsed();
        if (leftTime.isZero()) {
            // If we have no time left, we exit the loop
            LeaveCriticalSection(_crit());
            return false;
        }

        if (!SleepConditionVariableCS(_cond(), _crit(), leftTime.millis<u64>())) {
            LeaveCriticalSection(_crit());
            return false;
        }
    }

    LeaveCriticalSection(_crit());
    return true;
}

void Notify::notifyOne() {
    WakeConditionVariable(_cond());
}

void Notify::notifyAll() {
    WakeAllConditionVariable(_cond());
}

}