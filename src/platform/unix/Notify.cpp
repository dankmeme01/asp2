#include <asp/sync/Notify.hpp>
#include <asp/time/SystemTime.hpp>
#include <asp/Log.hpp>
#include <stdexcept>

namespace asp {

pthread_mutex_t* Notify::_mutex() {
    return reinterpret_cast<pthread_mutex_t*>(_storage);
}

pthread_cond_t* Notify::_cond() {
    return reinterpret_cast<pthread_cond_t*>(_storage + sizeof(pthread_mutex_t));
}

Notify::Notify() {
    pthread_mutex_init(_mutex(), nullptr);
    pthread_cond_init(_cond(), nullptr);
}

Notify::~Notify() {
    pthread_mutex_destroy(_mutex());
    pthread_cond_destroy(_cond());
}

void Notify::wait() {
    pthread_mutex_lock(_mutex());
    pthread_cond_wait(_cond(), _mutex());
    pthread_mutex_unlock(_mutex());
}

bool Notify::wait(const time::Duration& timeout) {
    // note: we rely on the fact that SystemTime also uses CLOCK_REALTIME

    auto expiry = time::SystemTime::now() + timeout;
    auto dur = expiry.timeSinceEpoch();

    struct timespec ts;
    ts.tv_sec = dur.seconds();
    ts.tv_nsec = dur.subsecNanos();

    pthread_mutex_lock(_mutex());
    int result = pthread_cond_timedwait(_cond(), _mutex(), &ts);
    pthread_mutex_unlock(_mutex());

    if (result != 0 && result != ETIMEDOUT) {
        // If the error is not ETIMEDOUT, we throw an exception
        asp::log(LogLevel::Error, "pthread_cond_timedwait failed with error: " + std::to_string(result));
        throw std::runtime_error("pthread_cond_timedwait failed");
    }

    return result == 0;
}

bool Notify::wait(const time::Duration& timeout, const std::function<bool()>& predicate) {
    pthread_mutex_lock(_mutex());

    if (timeout.isZero()) {
        // If timeout is zero, we wait indefinitely
        while (!predicate()) {
            pthread_cond_wait(_cond(), _mutex());
        }

        pthread_mutex_unlock(_mutex());
        return true;
    }

    auto expiry = time::SystemTime::now() + timeout;
    auto dur = expiry.timeSinceEpoch();

    struct timespec ts;
    ts.tv_sec = dur.seconds();
    ts.tv_nsec = dur.subsecNanos();

    while (!predicate()) {
        int result = pthread_cond_timedwait(_cond(), _mutex(), &ts);

        if (result == ETIMEDOUT) {
            pthread_mutex_unlock(_mutex());
            return false;
        } else if (result != 0) {
            pthread_mutex_unlock(_mutex());
            asp::log(LogLevel::Error, "pthread_cond_timedwait failed with error: " + std::to_string(result));
            throw std::runtime_error("pthread_cond_timedwait failed");
        }

        // keep checking the predicate
    }

    pthread_mutex_unlock(_mutex());
    return true;
}

void Notify::notifyOne() {
    pthread_cond_signal(_cond());
}

void Notify::notifyAll() {
    pthread_cond_broadcast(_cond());
}

}