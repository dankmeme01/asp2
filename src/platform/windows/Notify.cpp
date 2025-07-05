#include <asp/sync/Notify.hpp>
#include <asp/time/Instant.hpp>
#include <Windows.h>

static_assert(sizeof(CRITICAL_SECTION) + sizeof(CONDITION_VARIABLE) == ASP_NOTIFY_INNER_SIZE);

namespace asp {

CRITICAL_SECTION* Notify::_crit() {
    return reinterpret_cast<CRITICAL_SECTION*>(_storage);
}

CONDITION_VARIABLE* Notify::_cond() {
    return reinterpret_cast<CONDITION_VARIABLE*>(_storage + sizeof(CRITICAL_SECTION));
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
    bool result = SleepConditionVariableCS(_cond(), _crit(), timeout.millis<DWORD>());
    LeaveCriticalSection(_crit());
    return result;
}

bool Notify::wait(const time::Duration& timeout, const std::function<bool()>& predicate) {
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

        if (!SleepConditionVariableCS(_cond(), _crit(), leftTime.millis<DWORD>())) {
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