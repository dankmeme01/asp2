#include <asp/sync/DeadlockGuard.hpp>

#include <stdexcept>

namespace asp {

void DeadlockGuard::lockAttempt() {
    auto myId = std::this_thread::get_id();

    if (current == myId) {
        throw std::runtime_error("failed to lock mutex: already locked by this thread.");
    }

    current = myId;
}

void DeadlockGuard::lockSuccess() {
    current.store({}, std::memory_order::relaxed);
}

void DeadlockGuard::unlock() {
    current.store({}, std::memory_order::relaxed);
}

}